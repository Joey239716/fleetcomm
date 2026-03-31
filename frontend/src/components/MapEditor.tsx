import { useEffect, useRef, useState } from 'react'
import L from 'leaflet'
import { NODES, EDGES } from '../data/mapGraph'
import type { MapNode, MapEdge, ControlType } from '../data/mapGraph'

const LAT_M = 111320
const LNG_M = 87700
const EDGE_SNAP_M = 20

const GRID_OPTIONS = [
  { label: 'None',            value: 0       },
  { label: '0.00001°  (~1m)', value: 0.00001 },
  { label: '0.0001°  (~11m)', value: 0.0001  },
  { label: '0.001°  (~111m)', value: 0.001   },
]

type EditorMode = 'none' | 'addNode' | 'addEdge' | 'delEdge'

type UndoAction =
  | { type: 'addNode'; nodeId: string }
  | { type: 'addEdge'; edgeId: string }
  | { type: 'delEdge'; edge: MapEdge }

type InsertionState = {
  originalEdgeId: string
  originalFrom: string
  originalTo: string
  originalOneWay: boolean
  originalRoad: string
  originalSpeedLimit: number
  newEdge1Id: string
  newEdge2Id: string
}

function snap(v: number, grid: number): number {
  if (grid === 0) return v
  return Math.round(v / grid) * grid
}

function ptSegClosest(
  p: { lat: number; lng: number },
  a: { lat: number; lng: number },
  b: { lat: number; lng: number }
): { dist: number; point: { lat: number; lng: number } } {
  const dx = (b.lng - a.lng) * LNG_M
  const dy = (b.lat - a.lat) * LAT_M
  const len2 = dx * dx + dy * dy
  if (len2 === 0) {
    return { dist: Math.hypot((p.lat - a.lat) * LAT_M, (p.lng - a.lng) * LNG_M), point: a }
  }
  const t = Math.max(0, Math.min(1,
    ((p.lng - a.lng) * LNG_M * dx + (p.lat - a.lat) * LAT_M * dy) / len2
  ))
  const projLat = a.lat + t * (b.lat - a.lat)
  const projLng = a.lng + t * (b.lng - a.lng)
  return {
    dist: Math.hypot((p.lat - projLat) * LAT_M, (p.lng - projLng) * LNG_M),
    point: { lat: projLat, lng: projLng },
  }
}

function nodeColor(node: MapNode): string {
  return node.control === 'TrafficLight' ? '#6366f1' : '#f59e0b'
}

function makeDivIcon(node: MapNode): L.DivIcon {
  const color = nodeColor(node)
  return L.divIcon({
    className: '',
    html: `<div style="width:14px;height:14px;border-radius:50%;background:${color};border:2px solid #fff;box-shadow:0 1px 4px rgba(0,0,0,0.4);cursor:grab"></div>`,
    iconSize: [14, 14],
    iconAnchor: [7, 7],
  })
}

function makeSourceIcon(): L.DivIcon {
  return L.divIcon({
    className: '',
    html: `<div style="width:18px;height:18px;border-radius:50%;background:transparent;border:3px solid #22c55e;box-shadow:0 0 8px rgba(34,197,94,0.5);display:flex;align-items:center;justify-content:center"><div style="width:8px;height:8px;border-radius:50%;background:#22c55e"></div></div>`,
    iconSize: [18, 18],
    iconAnchor: [9, 9],
  })
}

type SaveStatus = 'idle' | 'saving' | 'saved' | 'error'

const GATEWAY_URL = (import.meta.env.VITE_WS_URL as string | undefined)
  ?.replace(/^ws/, 'http')
  ?.replace('/ws', '')
  ?? 'http://localhost:8080'

export default function MapEditor() {
  const containerRef      = useRef<HTMLDivElement>(null)
  const mapRef            = useRef<L.Map | null>(null)
  const markersRef        = useRef<Map<string, L.Marker>>(new Map())
  const polylinesRef      = useRef<Map<string, L.Polyline>>(new Map())
  const posRef            = useRef<Map<string, { lat: number; lng: number }>>(new Map())
  const allNodesRef       = useRef<MapNode[]>([...NODES])
  const edgesRef          = useRef<MapEdge[]>([...EDGES])
  const newNodeIdsRef     = useRef<Set<string>>(new Set())
  const insertionsRef     = useRef<Map<string, InsertionState>>(new Map())
  const undoStackRef      = useRef<UndoAction[]>([])
  const editorModeRef     = useRef<EditorMode>('none')
  const pendingEdgeFromRef = useRef<string | null>(null)
  const newControlRef     = useRef<ControlType>('StopSign')
  const edgeRoadRef       = useRef<string>('New Rd')
  const edgeOneWayRef     = useRef<boolean>(false)
  const gridRef           = useRef<number>(0)
  const nextIdRef         = useRef<number>(34)
  const edgeCtrRef        = useRef<number>(100)

  const [graphData, setGraphData]   = useState<{ nodes: MapNode[]; edges: MapEdge[] } | null>(null)
  const [editorMode, setEditorMode] = useState<EditorMode>('none')
  const [newControl, setNewControl] = useState<ControlType>('StopSign')
  const [edgeRoad, setEdgeRoad]     = useState('New Rd')
  const [edgeOneWay, setEdgeOneWay] = useState(false)
  const [gridSize, setGridSize]     = useState(0)
  const [saveStatus, setSaveStatus] = useState<SaveStatus>('idle')

  // Fetch current graph data from gateway on mount
  useEffect(() => {
    fetch(`${GATEWAY_URL}/map-data`)
      .then(r => r.json())
      .then(d => setGraphData(d))
      .catch(() => setGraphData({ nodes: NODES, edges: EDGES }))
  }, [])

  useEffect(() => { gridRef.current = gridSize }, [gridSize])
  useEffect(() => { newControlRef.current = newControl }, [newControl])
  useEffect(() => { edgeRoadRef.current = edgeRoad }, [edgeRoad])
  useEffect(() => { edgeOneWayRef.current = edgeOneWay }, [edgeOneWay])
  useEffect(() => {
    editorModeRef.current = editorMode
    if (mapRef.current) {
      mapRef.current.getContainer().style.cursor =
        editorMode === 'addNode' ? 'crosshair' : ''
    }
    // Cancel pending edge source when leaving addEdge mode
    if (editorMode !== 'addEdge' && pendingEdgeFromRef.current) {
      const srcId = pendingEdgeFromRef.current
      const srcNode = allNodesRef.current.find(n => n.id === srcId)
      if (srcNode) markersRef.current.get(srcId)?.setIcon(makeDivIcon(srcNode))
      pendingEdgeFromRef.current = null
    }
  }, [editorMode])

  // ── Edge helpers ───────────────────────────────────────────────────────────

  function addEdgeToMap(edge: MapEdge, posMap: Map<string, { lat: number; lng: number }>, map: L.Map) {
    const fromP = posMap.get(edge.from)!
    const toP   = posMap.get(edge.to)!
    const line  = L.polyline([[fromP.lat, fromP.lng], [toP.lat, toP.lng]], {
      color: '#64748b', weight: 2, opacity: 0.7,
    }).addTo(map)

    line.on('mouseover', () => {
      if (editorModeRef.current !== 'delEdge') return
      line.setStyle({ color: '#ef4444', weight: 4, opacity: 1 })
    })
    line.on('mouseout', () => {
      if (editorModeRef.current !== 'delEdge') return
      line.setStyle({ color: '#64748b', weight: 2, opacity: 0.7 })
    })
    line.on('click', (e: L.LeafletMouseEvent) => {
      if (editorModeRef.current !== 'delEdge') return
      L.DomEvent.stopPropagation(e)
      undoStackRef.current.push({ type: 'delEdge', edge })
      removeEdgeLine(edge.id)
    })

    polylinesRef.current.set(edge.id, line)
    edgesRef.current.push(edge)
  }

  function removeEdgeLine(edgeId: string) {
    polylinesRef.current.get(edgeId)?.remove()
    polylinesRef.current.delete(edgeId)
    edgesRef.current = edgesRef.current.filter(e => e.id !== edgeId)
  }

  function clearInsertion(nodeId: string, posMap: Map<string, { lat: number; lng: number }>, map: L.Map) {
    const ins = insertionsRef.current.get(nodeId)
    if (!ins) return
    removeEdgeLine(ins.newEdge1Id)
    removeEdgeLine(ins.newEdge2Id)
    addEdgeToMap({
      id: ins.originalEdgeId, road: ins.originalRoad,
      from: ins.originalFrom, to: ins.originalTo,
      oneWay: ins.originalOneWay, speedLimit: ins.originalSpeedLimit,
    }, posMap, map)
    insertionsRef.current.delete(nodeId)
  }

  function findClosestEdge(
    p: { lat: number; lng: number },
    excludeNodeId: string,
    posMap: Map<string, { lat: number; lng: number }>
  ): { edge: MapEdge; point: { lat: number; lng: number } } | null {
    let best: { edge: MapEdge; point: { lat: number; lng: number }; dist: number } | null = null
    for (const edge of edgesRef.current) {
      if (edge.from === excludeNodeId || edge.to === excludeNodeId) continue
      const a = posMap.get(edge.from)
      const b = posMap.get(edge.to)
      if (!a || !b) continue
      const { dist, point } = ptSegClosest(p, a, b)
      if (dist < EDGE_SNAP_M && (!best || dist < best.dist)) {
        best = { edge, point, dist }
      }
    }
    return best ? { edge: best.edge, point: best.point } : null
  }

  // ── Marker helper ─────────────────────────────────────────────────────────

  function spawnMarker(
    node: MapNode,
    map: L.Map,
    posMap: Map<string, { lat: number; lng: number }>,
    isNew: boolean
  ) {
    const pos    = posMap.get(node.id)!
    const marker = L.marker([pos.lat, pos.lng], {
      icon: makeDivIcon(node),
      draggable: true,
    })
      .bindTooltip(`${node.id}: ${node.name}`, { direction: 'top', offset: [0, -10] })
      .addTo(map)

    marker.on('drag', (e: L.LeafletEvent) => {
      const latlng = (e.target as L.Marker).getLatLng()
      posMap.set(node.id, { lat: latlng.lat, lng: latlng.lng })
      edgesRef.current.forEach(edge => {
        if (edge.from === node.id || edge.to === node.id) {
          const fromP = posMap.get(edge.from)!
          const toP   = posMap.get(edge.to)!
          polylinesRef.current.get(edge.id)?.setLatLngs([[fromP.lat, fromP.lng], [toP.lat, toP.lng]])
        }
      })
    })

    marker.on('dragend', () => {
      const raw = marker.getLatLng()
      let finalLat = snap(raw.lat, gridRef.current)
      let finalLng = snap(raw.lng, gridRef.current)

      if (isNew) {
        clearInsertion(node.id, posMap, map)
        const hit = findClosestEdge({ lat: finalLat, lng: finalLng }, node.id, posMap)
        if (hit) {
          finalLat = hit.point.lat
          finalLng = hit.point.lng
          marker.setLatLng([finalLat, finalLng])
          posMap.set(node.id, { lat: finalLat, lng: finalLng })

          const id1 = `E${edgeCtrRef.current++}`
          const id2 = `E${edgeCtrRef.current++}`
          removeEdgeLine(hit.edge.id)
          addEdgeToMap({ id: id1, road: hit.edge.road, from: hit.edge.from, to: node.id,   oneWay: hit.edge.oneWay, speedLimit: hit.edge.speedLimit }, posMap, map)
          addEdgeToMap({ id: id2, road: hit.edge.road, from: node.id,   to: hit.edge.to, oneWay: hit.edge.oneWay, speedLimit: hit.edge.speedLimit }, posMap, map)

          insertionsRef.current.set(node.id, {
            originalEdgeId: hit.edge.id,
            originalFrom: hit.edge.from,
            originalTo: hit.edge.to,
            originalOneWay: hit.edge.oneWay,
            originalRoad: hit.edge.road,
            originalSpeedLimit: hit.edge.speedLimit,
            newEdge1Id: id1,
            newEdge2Id: id2,
          })
        }
      }

      marker.setLatLng([finalLat, finalLng])
      posMap.set(node.id, { lat: finalLat, lng: finalLng })
      edgesRef.current.forEach(edge => {
        if (edge.from === node.id || edge.to === node.id) {
          const fromP = posMap.get(edge.from)!
          const toP   = posMap.get(edge.to)!
          polylinesRef.current.get(edge.id)?.setLatLngs([[fromP.lat, fromP.lng], [toP.lat, toP.lng]])
        }
      })
    })

    // Handle node click for Add Edge mode
    marker.on('click', () => {
      if (editorModeRef.current !== 'addEdge') return

      const pending = pendingEdgeFromRef.current

      if (!pending) {
        pendingEdgeFromRef.current = node.id
        marker.setIcon(makeSourceIcon())
      } else if (pending === node.id) {
        pendingEdgeFromRef.current = null
        marker.setIcon(makeDivIcon(node))
      } else {
        const srcNode = allNodesRef.current.find(n => n.id === pending)!
        const edgeId  = `E${edgeCtrRef.current++}`
        const newEdge: MapEdge = {
          id: edgeId,
          road: edgeRoadRef.current,
          from: pending,
          to: node.id,
          oneWay: edgeOneWayRef.current,
          speedLimit: 40,
        }
        addEdgeToMap(newEdge, posMap, map)
        undoStackRef.current.push({ type: 'addEdge', edgeId })
        markersRef.current.get(pending)?.setIcon(makeDivIcon(srcNode))
        pendingEdgeFromRef.current = null
      }
    })

    markersRef.current.set(node.id, marker)
  }

  // ── Map init ──────────────────────────────────────────────────────────────

  useEffect(() => {
    if (!graphData || !containerRef.current || mapRef.current) return

    // Seed nextIdRef from the highest existing node number
    const maxNum = graphData.nodes.reduce((max, n) => {
      const num = parseInt(n.id.replace(/\D/g, ''), 10)
      return isNaN(num) ? max : Math.max(max, num)
    }, 33)
    nextIdRef.current  = maxNum + 1
    allNodesRef.current = [...graphData.nodes]
    edgesRef.current    = [...graphData.edges]

    const bounds = L.latLngBounds([37.783, -122.412], [37.801, -122.394])
    const map = L.map(containerRef.current, {
      zoomControl: true,
      maxBounds: bounds,
      maxBoundsViscosity: 1.0,
      minZoom: 14,
    }).setView([37.792, -122.402], 15)

    L.tileLayer('https://{s}.basemaps.cartocdn.com/light_all/{z}/{x}/{y}{r}.png', {
      attribution: '&copy; OpenStreetMap &copy; CARTO',
      subdomains: 'abcd',
      maxZoom: 20,
    }).addTo(map)

    mapRef.current = map

    const posMap = new Map<string, { lat: number; lng: number }>()
    graphData.nodes.forEach(n => posMap.set(n.id, { lat: n.lat, lng: n.lng }))
    posRef.current = posMap

    graphData.edges.forEach(edge => addEdgeToMap(edge, posMap, map))
    graphData.nodes.forEach(node => spawnMarker(node, map, posMap, false))

    map.on('click', (e: L.LeafletMouseEvent) => {
      if (editorModeRef.current !== 'addNode') return
      const lat  = snap(e.latlng.lat, gridRef.current)
      const lng  = snap(e.latlng.lng, gridRef.current)
      const id   = `N${nextIdRef.current++}`
      const node: MapNode = { id, name: id, lat, lng, control: newControlRef.current }
      posMap.set(id, { lat, lng })
      allNodesRef.current.push(node)
      newNodeIdsRef.current.add(id)
      undoStackRef.current.push({ type: 'addNode', nodeId: id })
      spawnMarker(node, map, posMap, true)
    })

    const handleKeydown = (e: KeyboardEvent) => {
      if (!(e.ctrlKey || e.metaKey) || e.key !== 'z') return
      e.preventDefault()
      const action = undoStackRef.current.pop()
      if (!action) return

      if (action.type === 'addNode') {
        clearInsertion(action.nodeId, posMap, map)
        markersRef.current.get(action.nodeId)?.remove()
        markersRef.current.delete(action.nodeId)
        posMap.delete(action.nodeId)
        allNodesRef.current = allNodesRef.current.filter(n => n.id !== action.nodeId)
        newNodeIdsRef.current.delete(action.nodeId)
      } else if (action.type === 'addEdge') {
        removeEdgeLine(action.edgeId)
      } else if (action.type === 'delEdge') {
        addEdgeToMap(action.edge, posMap, map)
      }
    }

    document.addEventListener('keydown', handleKeydown)

    return () => {
      document.removeEventListener('keydown', handleKeydown)
      map.remove()
      mapRef.current      = null
      markersRef.current.clear()
      polylinesRef.current.clear()
      edgesRef.current    = [...(graphData?.edges ?? [])]
      allNodesRef.current = [...(graphData?.nodes ?? [])]
      newNodeIdsRef.current.clear()
      insertionsRef.current.clear()
      undoStackRef.current = []
      nextIdRef.current   = 34
      edgeCtrRef.current  = 100
    }
  // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [graphData])

  // ── Save ──────────────────────────────────────────────────────────────────

  async function handleSave() {
    setSaveStatus('saving')
    const posMap = posRef.current
    const payload = {
      nodes: allNodesRef.current.map(n => ({
        id: n.id,
        name: n.name,
        lat: posMap.get(n.id)?.lat ?? n.lat,
        lng: posMap.get(n.id)?.lng ?? n.lng,
        control: n.control,
      })),
      edges: edgesRef.current.map(e => ({
        id: e.id,
        road: e.road,
        from: e.from,
        to: e.to,
        oneWay: e.oneWay,
        speedLimit: e.speedLimit,
      })),
    }
    try {
      const res = await fetch(`${GATEWAY_URL}/save-map`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload),
      })
      if (res.ok) {
        setSaveStatus('saved')
      } else {
        const body = await res.text()
        console.error(`[save-map] ${res.status}:`, body)
        setSaveStatus('error')
      }
    } catch (err) {
      console.error('[save-map] network error:', err)
      setSaveStatus('error')
    }
    setTimeout(() => setSaveStatus('idle'), 2000)
  }

  // ── UI helpers ────────────────────────────────────────────────────────────

  function toggleMode(m: EditorMode) {
    setEditorMode(cur => cur === m ? 'none' : m)
  }

  const btnBase: React.CSSProperties = {
    fontFamily: "'JetBrains Mono', monospace",
    fontSize: 11,
    fontWeight: 700,
    textTransform: 'uppercase',
    letterSpacing: '0.08em',
    padding: '4px 10px',
    borderRadius: 4,
    cursor: 'pointer',
    border: '1px solid',
    background: 'transparent',
  }

  function modeBtn(m: EditorMode, label: string, activeColor: string, activeBg: string): React.ReactElement {
    const active = editorMode === m
    return (
      <button
        key={m}
        onClick={() => toggleMode(m)}
        style={{
          ...btnBase,
          borderColor: active ? activeColor : 'rgba(0,0,0,0.15)',
          color: active ? activeColor : '#6e6e73',
          background: active ? activeBg : 'transparent',
        }}
      >
        {label}
      </button>
    )
  }

  const sep = <div style={{ width: 1, height: 22, background: 'rgba(0,0,0,0.12)', margin: '0 2px' }} />

  const saveLabel =
    saveStatus === 'saving' ? 'Saving...' :
    saveStatus === 'saved'  ? 'Saved!' :
    saveStatus === 'error'  ? 'Error' : 'Save'
  const saveColor =
    saveStatus === 'saved'  ? '#22c55e' :
    saveStatus === 'error'  ? '#ef4444' : '#ea580c'

  if (!graphData) return (
    <div style={{ width: '100%', height: '100%', display: 'flex', alignItems: 'center', justifyContent: 'center', fontFamily: "'JetBrains Mono', monospace", fontSize: 12, color: '#6e6e73' }}>
      Loading map data...
    </div>
  )

  return (
    <div style={{ width: '100%', height: '100%', position: 'relative' }}>
      <div ref={containerRef} style={{ width: '100%', height: '100%' }} />

      <div style={{
        position: 'absolute', bottom: 24, right: 12, zIndex: 1000,
        display: 'flex', alignItems: 'center', gap: 8, flexWrap: 'wrap',
        background: 'rgba(255,255,255,0.95)',
        backdropFilter: 'blur(12px)',
        border: '1px solid rgba(0,0,0,0.1)',
        borderRadius: 6,
        padding: '8px 12px',
        boxShadow: '0 2px 12px rgba(0,0,0,0.12)',
        fontFamily: "'JetBrains Mono', monospace",
        fontSize: 11,
        maxWidth: 'calc(100vw - 24px)',
      }}>
        {/* Grid */}
        <label style={{ color: '#6e6e73', fontWeight: 700, textTransform: 'uppercase', letterSpacing: '0.08em' }}>
          Grid
        </label>
        <select
          value={gridSize}
          onChange={e => setGridSize(Number(e.target.value))}
          style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 11, padding: '3px 6px', border: '1px solid rgba(0,0,0,0.2)', borderRadius: 4, background: 'white', cursor: 'pointer' }}
        >
          {GRID_OPTIONS.map(opt => <option key={opt.value} value={opt.value}>{opt.label}</option>)}
        </select>

        {sep}

        {/* Mode buttons */}
        {modeBtn('addNode', 'Add Node', '#22c55e',  'rgba(34,197,94,0.08)')}
        {modeBtn('addEdge', 'Add Edge', '#3b82f6',  'rgba(59,130,246,0.08)')}
        {modeBtn('delEdge', 'Del Edge', '#ef4444',  'rgba(239,68,68,0.08)')}

        {/* Context controls */}
        {editorMode === 'addNode' && (<>
          {sep}
          <select
            value={newControl}
            onChange={e => setNewControl(e.target.value as ControlType)}
            style={{
              fontFamily: "'JetBrains Mono', monospace", fontSize: 11, padding: '3px 6px',
              border: `1px solid ${newControl === 'TrafficLight' ? 'rgba(99,102,241,0.4)' : 'rgba(245,158,11,0.4)'}`,
              borderRadius: 4, background: 'white', cursor: 'pointer',
              color: newControl === 'TrafficLight' ? '#6366f1' : '#f59e0b',
            }}
          >
            <option value="StopSign">Stop Sign</option>
            <option value="TrafficLight">Traffic Light</option>
          </select>
        </>)}

        {editorMode === 'addEdge' && (<>
          {sep}
          <input
            value={edgeRoad}
            onChange={e => setEdgeRoad(e.target.value)}
            placeholder="Road name"
            style={{
              fontFamily: "'JetBrains Mono', monospace", fontSize: 11, padding: '3px 8px',
              border: '1px solid rgba(0,0,0,0.2)', borderRadius: 4, width: 110,
            }}
          />
          <label style={{ display: 'flex', alignItems: 'center', gap: 4, color: '#6e6e73', cursor: 'pointer', userSelect: 'none' }}>
            <input
              type="checkbox"
              checked={edgeOneWay}
              onChange={e => setEdgeOneWay(e.target.checked)}
              style={{ cursor: 'pointer' }}
            />
            One-way
          </label>
        </>)}

        {sep}

        {/* Save */}
        <button
          onClick={handleSave}
          disabled={saveStatus === 'saving'}
          style={{
            ...btnBase,
            borderColor: `${saveColor}66`,
            color: saveColor,
            background: `${saveColor}14`,
          }}
        >
          {saveLabel}
        </button>
      </div>
    </div>
  )
}
