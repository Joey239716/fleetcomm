package main

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"regexp"
	"strconv"
	"strings"
	"sync"

	"github.com/go-zeromq/zmq4"
	"github.com/gorilla/websocket"
	"google.golang.org/protobuf/proto"

	pb "fleetcomm/gateway/gen/fleetcomm"
)

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool { return true },
}

type carJSON struct {
	ID              int32   `json:"id"`
	Lat             float64 `json:"lat"`
	Lng             float64 `json:"lng"`
	Speed           float32 `json:"spd"`
	Heading         float32 `json:"hdg"`
	Status          int32   `json:"st"`
	BrakingIntensity float32 `json:"braking_intensity"`
}

type worldJSON struct {
	Tick int32     `json:"tick"`
	Mode string    `json:"mode"`
	Cars []carJSON `json:"cars"`
}

type modeCmd struct {
	Mode string `json:"mode"`
}

// cmdPush sends mode commands to the C++ sim.
var (
	cmdPush     zmq4.Socket
	cmdPushOnce sync.Once
)

func initCmdPush(ctx context.Context) {
	cmdPushOnce.Do(func() {
		cmdPush = zmq4.NewPush(ctx)
		if err := cmdPush.Dial("tcp://localhost:5556"); err != nil {
			log.Printf("zmq cmd push dial: %v (sim not running?)", err)
		}
		log.Println("ZMQ command push connected to tcp://localhost:5556")
	})
}

func sendModeCommand(mode string) {
	msg := zmq4.NewMsgString(mode)
	if err := cmdPush.Send(msg); err != nil {
		log.Printf("zmq cmd send: %v", err)
	}
}

func zmqSubscriber(hub *Hub) {
	ctx := context.Background()

	initCmdPush(ctx)

	sub := zmq4.NewSub(ctx)
	defer sub.Close()

	if err := sub.Dial("tcp://localhost:5555"); err != nil {
		log.Printf("zmq dial: %v (sim not running?)", err)
		return
	}
	if err := sub.SetOption(zmq4.OptionSubscribe, ""); err != nil {
		log.Printf("zmq subscribe: %v (sim not running?)", err)
		return
	}

	log.Println("ZMQ subscriber connected to tcp://localhost:5555")

	for {
		msg, err := sub.Recv()
		if err != nil {
			log.Printf("zmq recv: %v", err)
			return
		}

		var ws pb.WorldState
		if err := proto.Unmarshal(msg.Frames[0], &ws); err != nil {
			log.Printf("proto unmarshal: %v", err)
			continue
		}

		cars := make([]carJSON, 0, len(ws.Cars))
		for _, c := range ws.Cars {
			cars = append(cars, carJSON{
				ID:              c.Id,
				Lat:             c.Lat,
				Lng:             c.Lng,
				Speed:           c.Speed,
				Heading:         c.Heading,
				Status:          c.Status,
				BrakingIntensity: c.BrakingIntensity,
			})
		}

		payload, err := json.Marshal(worldJSON{
			Tick: ws.Tick,
			Mode: ws.Mode,
			Cars: cars,
		})
		if err != nil {
			log.Printf("json marshal: %v", err)
			continue
		}

		hub.broadcast(payload)
	}
}

func wsHandler(hub *Hub, w http.ResponseWriter, r *http.Request) {
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Printf("ws upgrade: %v", err)
		return
	}
	hub.register(conn)
	defer hub.unregister(conn)

	for {
		_, raw, err := conn.ReadMessage()
		if err != nil {
			break
		}
		var cmd modeCmd
		if err := json.Unmarshal(raw, &cmd); err != nil {
			continue
		}
		if cmd.Mode == "human" || cmd.Mode == "v2v" {
			sendModeCommand(cmd.Mode)
		}
	}
}

type nodeJSON struct {
	ID      string  `json:"id"`
	Name    string  `json:"name"`
	Lat     float64 `json:"lat"`
	Lng     float64 `json:"lng"`
	Control string  `json:"control"`
}

type edgeJSON struct {
	ID         string  `json:"id"`
	Road       string  `json:"road"`
	From       string  `json:"from"`
	To         string  `json:"to"`
	OneWay     bool    `json:"oneWay"`
	SpeedLimit float64 `json:"speedLimit"`
}

type saveMapPayload struct {
	Nodes []nodeJSON `json:"nodes"`
	Edges []edgeJSON `json:"edges"`
}

var (
	nodeRe = regexp.MustCompile(`addNode\(\{"([^"]+)",\s*"([^"]+)",\s*([\d.-]+)f,\s*([\d.-]+)f,\s*(ControlType::\w+)\}\)`)
	edgeRe = regexp.MustCompile(`addEdge\(\{"([^"]+)",\s*"([^"]+)",\s*haversineDistance\([^)]+\),\s*"([^"]+)",\s*"([^"]+)",\s*[^,]+,\s*[^,]+,\s*([\d.]+)f,\s*(true|false)\}\)`)
)

func parseGraphCpp(src string) ([]nodeJSON, []edgeJSON) {
	var nodes []nodeJSON
	for _, m := range nodeRe.FindAllStringSubmatch(src, -1) {
		lat, _ := strconv.ParseFloat(m[3], 64)
		lng, _ := strconv.ParseFloat(m[4], 64)
		ctrl := "StopSign"
		if m[5] == "ControlType::TrafficLight" {
			ctrl = "TrafficLight"
		}
		nodes = append(nodes, nodeJSON{ID: m[1], Name: m[2], Lat: lat, Lng: lng, Control: ctrl})
	}
	var edges []edgeJSON
	for _, m := range edgeRe.FindAllStringSubmatch(src, -1) {
		speedLimit, _ := strconv.ParseFloat(m[5], 64)
		edges = append(edges, edgeJSON{ID: m[1], Road: m[2], From: m[3], To: m[4], SpeedLimit: speedLimit, OneWay: m[6] == "true"})
	}
	return nodes, edges
}

func mapDataHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Access-Control-Allow-Origin", "*")
	graphPath := os.Getenv("GRAPH_CPP_PATH")
	if graphPath == "" {
		graphPath = "../src/graph.cpp"
	}
	src, err := os.ReadFile(graphPath)
	if err != nil {
		http.Error(w, "cannot read graph.cpp: "+err.Error(), http.StatusInternalServerError)
		return
	}
	nodes, edges := parseGraphCpp(string(src))
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(struct {
		Nodes []nodeJSON `json:"nodes"`
		Edges []edgeJSON `json:"edges"`
	}{nodes, edges})
}

func saveMapHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Access-Control-Allow-Origin", "*")
	w.Header().Set("Access-Control-Allow-Methods", "POST, OPTIONS")
	w.Header().Set("Access-Control-Allow-Headers", "Content-Type")
	if r.Method == http.MethodOptions {
		w.WriteHeader(http.StatusNoContent)
		return
	}
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var payload saveMapPayload
	if err := json.NewDecoder(r.Body).Decode(&payload); err != nil {
		http.Error(w, "bad request: "+err.Error(), http.StatusBadRequest)
		return
	}

	graphPath := os.Getenv("GRAPH_CPP_PATH")
	if graphPath == "" {
		graphPath = "../src/graph.cpp"
	}

	src, err := os.ReadFile(graphPath)
	if err != nil {
		http.Error(w, "cannot read graph.cpp: "+err.Error(), http.StatusInternalServerError)
		return
	}

	const marker = "void RoadGraph::buildHardcodedMap() {"
	idx := strings.Index(string(src), marker)
	if idx == -1 {
		http.Error(w, "buildHardcodedMap not found in graph.cpp", http.StatusInternalServerError)
		return
	}

	var sb strings.Builder
	sb.WriteString(string(src[:idx+len(marker)]))
	sb.WriteString("\n")

	for _, n := range payload.Nodes {
		ctrl := "ControlType::StopSign"
		if n.Control == "TrafficLight" {
			ctrl = "ControlType::TrafficLight"
		}
		fmt.Fprintf(&sb, "    addNode({\"%s\", \"%s\", %.6ff, %.6ff, %s});\n",
			n.ID, n.Name, n.Lat, n.Lng, ctrl)
	}

	sb.WriteString("\n")

	for _, e := range payload.Edges {
		oneWay := "false"
		if e.OneWay {
			oneWay = "true"
		}
		fmt.Fprintf(&sb, "    addEdge({\"%s\", \"%s\", haversineDistance(nodes[\"%s\"], nodes[\"%s\"]), \"%s\", \"%s\", 0.0f, false, %.1ff, %s});\n",
			e.ID, e.Road, e.From, e.To, e.From, e.To, e.SpeedLimit, oneWay)
	}

	sb.WriteString("}\n")

	if err := os.WriteFile(graphPath, []byte(sb.String()), 0644); err != nil {
		http.Error(w, "cannot write graph.cpp: "+err.Error(), http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	w.Write([]byte(`{"ok":true}`))
}

func main() {
	hub := newHub()

	go zmqSubscriber(hub)

	http.HandleFunc("/ws", func(w http.ResponseWriter, r *http.Request) {
		wsHandler(hub, w, r)
	})
	http.HandleFunc("/map-data", mapDataHandler)
	http.HandleFunc("/save-map", saveMapHandler)
	http.Handle("/", http.FileServer(http.Dir("./static")))

	fmt.Println("Gateway listening on :8080  →  open http://localhost:8080")
	log.Fatal(http.ListenAndServe(":8080", nil))
}
