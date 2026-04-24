import { useEffect, useRef } from 'react'
import L from 'leaflet'
import type { CarState, Mode } from '../types'

// How fast rendered position chases target — higher = snappier, lower = smoother
const LERP = 0.18
const TRAIL_MAX = 60 // ~2 seconds of server frames at 30fps

function brakingColor(intensity: number, mode: Mode): string {
  if (mode === 'human') {
    const r = Math.round(255 + (224 - 255) * intensity)
    const g = Math.round(107 + (58  - 107) * intensity)
    const b = Math.round(53  + (58  - 53)  * intensity)
    return `rgb(${r},${g},${b})`
  } else {
    const r = Math.round(45  + (224 - 45)  * intensity)
    const g = Math.round(184 + (58  - 184) * intensity)
    const b = Math.round(75  + (58  - 75)  * intensity)
    return `rgb(${r},${g},${b})`
  }
}

interface RenderState {
  lat: number
  lng: number
  targetLat: number
  targetLng: number
  hdg: number
  targetHdg: number
  braking_intensity: number
  st: number
  spd: number
}

function lerpAngle(current: number, target: number, t: number): number {
  const delta = ((target - current + 540) % 360) - 180
  return current + delta * t
}

function carIconHtml(hdg: number, color: string): string {
  return `<div style="width:10px;height:14px;transform:rotate(${hdg}deg);transform-origin:center;color:${color};filter:drop-shadow(0 0 1px rgba(0,0,0,0.4))"><svg width="10" height="14" viewBox="0 0 10 14" xmlns="http://www.w3.org/2000/svg"><polygon points="5,0 10,14 5,10 0,14" fill="currentColor" stroke="white" stroke-width="1.2" stroke-linejoin="round"/></svg></div>`
}

interface TLOverlay {
  // green: one polyline through the node connecting both green-phase approaches
  nsGreen: L.Polyline
  ewGreen: L.Polyline
  // red: individual stubs stopping at the node for each blocked approach
  nsRed: L.Polyline[]
  ewRed: L.Polyline[]
}

interface Props {
  cars: CarState[]
  mode: Mode
  showTrail: boolean
  tick: number
}

const GATEWAY_URL = (import.meta.env.VITE_WS_URL as string | undefined)
  ?.replace(/^ws/, 'http')?.replace('/ws', '') ?? 'http://localhost:8080'

const OVERLAY_M = 20 // metres each side of node

function pointToward(
  from: { lat: number; lng: number },
  toward: { lat: number; lng: number },
  metres: number
): [number, number] {
  const dlat = toward.lat - from.lat
  const dlng = toward.lng - from.lng
  const cosLat = Math.cos((from.lat * Math.PI) / 180)
  const distDeg = Math.sqrt(dlat * dlat + (dlng * cosLat) * (dlng * cosLat))
  if (distDeg === 0) return [from.lat, from.lng]
  const scale = metres / 111320 / distDeg
  return [from.lat + dlat * scale, from.lng + dlng * scale]
}

export default function FleetMap({ cars, mode, showTrail, tick }: Props) {
  const containerRef   = useRef<HTMLDivElement>(null)
  const mapRef         = useRef<L.Map | null>(null)
  const markersRef     = useRef<Map<number, L.Marker>>(new Map())
  const targetDotsRef  = useRef<Map<number, L.CircleMarker>>(new Map())
  const trailPointsRef = useRef<Map<number, [number, number][]>>(new Map())
  const trailLinesRef  = useRef<Map<number, L.Polyline>>(new Map())
  const stateRef       = useRef<Map<number, RenderState>>(new Map())
  const rafRef         = useRef<number>(0)
  const modeRef        = useRef<Mode>(mode)
  const showTrailRef   = useRef<boolean>(showTrail)
  const tlOverlaysRef  = useRef<Map<string, TLOverlay>>(new Map())
  const lastPhaseRef   = useRef<number>(-1)

  // Keep refs in sync with props
  useEffect(() => { showTrailRef.current = showTrail }, [showTrail])

  // Initialise map + start RAF loop once
  useEffect(() => {
    if (!containerRef.current || mapRef.current) return

    const bounds = L.latLngBounds([37.783, -122.412], [37.801, -122.394])
    const map = L.map(containerRef.current, {
      zoomControl: false,
      minZoom: 14,
    }).setView([37.792, -122.400], 16)
    map.setMinZoom(14)
    const updateBounds = () => {
      if (map.getZoom() <= 14) {
        map.setMaxBounds(bounds)
      } else {
        map.setMaxBounds(null as unknown as L.LatLngBounds)
      }
    }
    map.on('zoomend', updateBounds)
    updateBounds()

    L.tileLayer('https://{s}.basemaps.cartocdn.com/light_all/{z}/{x}/{y}{r}.png', {
      attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> &copy; <a href="https://carto.com/attributions">CARTO</a>',
      subdomains: 'abcd',
      maxZoom: 20,
    }).addTo(map)

    mapRef.current = map

    const render = () => {
      const map = mapRef.current
      if (!map) return
      const seenIds = new Set<number>()

      stateRef.current.forEach((s, id) => {
        seenIds.add(id)

        // Exponential smoothing — nudge rendered pos and heading toward target each frame
        s.lat += (s.targetLat - s.lat) * LERP
        s.lng += (s.targetLng - s.lng) * LERP
        s.hdg  = lerpAngle(s.hdg, s.targetHdg, LERP)

        const color = s.st === 2 ? '#E03A3A' : brakingColor(s.braking_intensity, modeRef.current)

        if (markersRef.current.has(id)) {
          const m = markersRef.current.get(id)!
          m.setLatLng([s.lat, s.lng])
          const inner = m.getElement()?.firstElementChild as HTMLElement | null
          if (inner) {
            inner.style.transform = `rotate(${s.hdg}deg)`
            inner.style.color = color
          }
        } else {
          const icon = L.divIcon({
            html: carIconHtml(s.hdg, color),
            className: '',
            iconSize: [10, 14],
            iconAnchor: [5, 7],
          })
          const m = L.marker([s.lat, s.lng], { icon })
            .bindTooltip(`Car ${id} · ${Math.round(s.spd)} km/h`, { direction: 'top', offset: [0, -8] })
            .addTo(map)
          markersRef.current.set(id, m)
        }

        // Raw server-position dot (shown when trail debug is on)
        if (showTrailRef.current) {
          if (targetDotsRef.current.has(id)) {
            targetDotsRef.current.get(id)!.setLatLng([s.targetLat, s.targetLng])
          } else {
            const dot = L.circleMarker([s.targetLat, s.targetLng], {
              radius: 3, fillColor: '#a855f7', color: '#ffffff', weight: 1, fillOpacity: 0.9,
            }).addTo(map)
            targetDotsRef.current.set(id, dot)
          }
        } else if (targetDotsRef.current.has(id)) {
          targetDotsRef.current.get(id)!.remove()
          targetDotsRef.current.delete(id)
        }
      })

      // Remove stale markers
      markersRef.current.forEach((marker, id) => {
        if (!seenIds.has(id)) {
          marker.remove()
          markersRef.current.delete(id)
        }
      })
      targetDotsRef.current.forEach((dot, id) => {
        if (!seenIds.has(id)) {
          dot.remove()
          targetDotsRef.current.delete(id)
        }
      })

      rafRef.current = requestAnimationFrame(render)
    }

    rafRef.current = requestAnimationFrame(render)

    return () => {
      cancelAnimationFrame(rafRef.current)
      map.remove()
      mapRef.current = null
    }
  }, [])

  // Fetch map data once and build traffic-light overlays
  useEffect(() => {
    const map = mapRef.current
    if (!map) return

    fetch(`${GATEWAY_URL}/map-data`)
      .then(r => r.json())
      .then(({ nodes, edges }: { nodes: Array<{ id: string; lat: number; lng: number; control: string }>; edges: Array<{ from: string; to: string; oneWay: boolean }> }) => {
        const nodeMap = new Map(nodes.map(n => [n.id, n]))

        for (const node of nodes) {
          if (node.control !== 'TrafficLight') continue

          // Collect all neighbour positions reachable from this node
          const neighbours: Array<{ lat: number; lng: number }> = []
          for (const e of edges) {
            if (e.from === node.id) neighbours.push(nodeMap.get(e.to)!)
            else if (e.to === node.id) neighbours.push(nodeMap.get(e.from)!)
          }

          const cosLat = Math.cos((node.lat * Math.PI) / 180)
          const nsEps: [number, number][] = []
          const ewEps: [number, number][] = []

          for (const nb of neighbours) {
            if (!nb) continue
            const dlat = nb.lat - node.lat
            const dlng = nb.lng - node.lng
            const ep = pointToward(node, nb, OVERLAY_M)
            if (Math.abs(dlat) >= Math.abs(dlng * cosLat)) nsEps.push(ep)
            else ewEps.push(ep)
          }

          const nodePt: [number, number] = [node.lat, node.lng]

          // Green: mirror through node if only one real approach so the line passes through
          const greenPair = (eps: [number, number][]): [number, number][] => {
            if (eps.length >= 2) return [eps[0], eps[eps.length - 1]]
            if (eps.length === 1) return [eps[0], [2 * node.lat - eps[0][0], 2 * node.lng - eps[0][1]]]
            return []
          }

          const nsGreenPts = greenPair(nsEps)
          const ewGreenPts = greenPair(ewEps)

          const polyOpts = (color: string): L.PolylineOptions => ({
            color, weight: 11, opacity: 0.3, lineJoin: 'round', lineCap: 'round',
          })

          const nsGreen = nsGreenPts.length === 2
            ? L.polyline([nsGreenPts[0], nodePt, nsGreenPts[1]], polyOpts('#86efac')).addTo(map)
            : L.polyline([], polyOpts('#86efac')).addTo(map)

          const ewGreen = ewGreenPts.length === 2
            ? L.polyline([ewGreenPts[0], nodePt, ewGreenPts[1]], polyOpts('#86efac')).addTo(map)
            : L.polyline([], polyOpts('#86efac')).addTo(map)

          // Red stubs: only draw for actual graph neighbours — no mirroring
          // Stop 3m short of the node so they don't bleed into the intersection
          const RED_GAP_M = 3
          const nsRed = nsEps.map(ep =>
            L.polyline([ep, pointToward(node, { lat: ep[0], lng: ep[1] }, RED_GAP_M)], polyOpts('#fca5a5')).addTo(map)
          )
          const ewRed = ewEps.map(ep =>
            L.polyline([ep, pointToward(node, { lat: ep[0], lng: ep[1] }, RED_GAP_M)], polyOpts('#fca5a5')).addTo(map)
          )

          tlOverlaysRef.current.set(node.id, { nsGreen, ewGreen, nsRed, ewRed })
        }

        // Apply initial phase (phase 0 = N-S green)
        applyPhase(0)
      })
      .catch(() => { /* sim not running — overlays stay empty */ })
  // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [])

  function applyPhase(phase: number) {
    tlOverlaysRef.current.forEach(({ nsGreen, ewGreen, nsRed, ewRed }) => {
      if (phase === 0) {
        nsGreen.setStyle({ opacity: 0.55 })
        ewGreen.setStyle({ opacity: 0 })
        nsRed.forEach(p => p.setStyle({ opacity: 0 }))
        ewRed.forEach(p => p.setStyle({ opacity: 0.55 }))
      } else {
        ewGreen.setStyle({ opacity: 0.55 })
        nsGreen.setStyle({ opacity: 0 })
        ewRed.forEach(p => p.setStyle({ opacity: 0 }))
        nsRed.forEach(p => p.setStyle({ opacity: 0.55 }))
      }
    })
  }

  // Update overlay colours when phase changes
  useEffect(() => {
    if (tlOverlaysRef.current.size === 0) return
    const phase = Math.floor(tick / 150) % 2
    if (phase === lastPhaseRef.current) return
    lastPhaseRef.current = phase
    applyPhase(phase)
  // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [tick])

  // When a new server frame arrives, update targets and trails
  useEffect(() => {
    modeRef.current = mode
    const map = mapRef.current
    const incoming = new Set<number>()

    cars.forEach(c => {
      incoming.add(c.id)
      const existing = stateRef.current.get(c.id)
      if (existing) {
        // Detect backward movement in raw server data
        const prevLat = existing.targetLat
        const prevLng = existing.targetLng
        const dlat = c.lat - prevLat
        const dlng = c.lng - prevLng
        const distM = Math.sqrt(dlat * dlat + dlng * dlng) * 111320
        if (distM > 0.3) { // moved at least 0.3m
          // Estimate whether car moved backward by comparing to heading
          const hdgRad = (c.hdg * Math.PI) / 180
          const dotForward = dlat * Math.cos(hdgRad) + dlng * Math.sin(hdgRad)
          if (dotForward < -0.5) {
            console.warn(`[BACKWARD] Car ${c.id}  spd=${c.spd.toFixed(1)} hdg=${c.hdg.toFixed(0)}° ` +
              `from (${prevLat.toFixed(6)},${prevLng.toFixed(6)}) ` +
              `→ (${c.lat.toFixed(6)},${c.lng.toFixed(6)})  Δ=${distM.toFixed(1)}m`)
          }
        }
        existing.targetLat = c.lat
        existing.targetLng = c.lng
        existing.targetHdg = c.hdg
        existing.braking_intensity = c.braking_intensity ?? 0
        existing.st  = c.st
        existing.spd = c.spd
      } else {
        stateRef.current.set(c.id, {
          lat: c.lat, lng: c.lng,
          targetLat: c.lat, targetLng: c.lng,
          hdg: c.hdg, targetHdg: c.hdg,
          braking_intensity: c.braking_intensity ?? 0,
          st: c.st, spd: c.spd,
        })
      }

      // Update trail for this car
      if (showTrail && map) {
        const pts = trailPointsRef.current.get(c.id) ?? []
        pts.push([c.lat, c.lng])
        if (pts.length > TRAIL_MAX) pts.shift()
        trailPointsRef.current.set(c.id, pts)

        if (trailLinesRef.current.has(c.id)) {
          trailLinesRef.current.get(c.id)!.setLatLngs(pts)
        } else {
          const line = L.polyline(pts, { color: '#a855f7', weight: 1.5, opacity: 0.6 }).addTo(map)
          trailLinesRef.current.set(c.id, line)
        }
      }
    })

    // Clear trails when showTrail is toggled off
    if (!showTrail) {
      trailLinesRef.current.forEach(line => line.remove())
      trailLinesRef.current.clear()
      trailPointsRef.current.clear()
    }

    // Remove cars that left the frame
    stateRef.current.forEach((_, id) => {
      if (!incoming.has(id)) {
        stateRef.current.delete(id)
        trailPointsRef.current.delete(id)
        if (trailLinesRef.current.has(id)) {
          trailLinesRef.current.get(id)!.remove()
          trailLinesRef.current.delete(id)
        }
      }
    })
  }, [cars, mode, showTrail])

  return <div ref={containerRef} style={{ width: '100%', height: '100%' }} />
}
