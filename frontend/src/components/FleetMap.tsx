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
  // Current rendered position (smoothed)
  lat: number
  lng: number
  // Target position from latest server frame
  targetLat: number
  targetLng: number
  braking_intensity: number
  st: number
  spd: number
}

interface Props {
  cars: CarState[]
  mode: Mode
  showTrail: boolean
}

export default function FleetMap({ cars, mode, showTrail }: Props) {
  const containerRef   = useRef<HTMLDivElement>(null)
  const mapRef         = useRef<L.Map | null>(null)
  const markersRef     = useRef<Map<number, L.CircleMarker>>(new Map())
  const targetDotsRef  = useRef<Map<number, L.CircleMarker>>(new Map())
  const trailPointsRef = useRef<Map<number, [number, number][]>>(new Map())
  const trailLinesRef  = useRef<Map<number, L.Polyline>>(new Map())
  const stateRef       = useRef<Map<number, RenderState>>(new Map())
  const rafRef         = useRef<number>(0)
  const modeRef        = useRef<Mode>(mode)
  const showTrailRef   = useRef<boolean>(showTrail)

  // Keep refs in sync with props
  useEffect(() => { showTrailRef.current = showTrail }, [showTrail])

  // Initialise map + start RAF loop once
  useEffect(() => {
    if (!containerRef.current || mapRef.current) return

    const bounds = L.latLngBounds([37.783, -122.395], [37.801, -122.394])
    const map = L.map(containerRef.current, {
      zoomControl: false,
      maxBounds: bounds,
      maxBoundsViscosity: 1.0,
      minZoom: 14,
    }).setView([37.792, -122.400], 16)
    map.setMinZoom(14)
    map.on('zoomend', () => { if (map.getZoom() < 14) map.setZoom(14, { animate: false }) })

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

        // Exponential smoothing — nudge rendered pos toward target each frame
        s.lat += (s.targetLat - s.lat) * LERP
        s.lng += (s.targetLng - s.lng) * LERP

        const color = s.st === 2 ? '#E03A3A' : brakingColor(s.braking_intensity, modeRef.current)

        // Smooth dot (lerped position)
        if (markersRef.current.has(id)) {
          const m = markersRef.current.get(id)!
          m.setLatLng([s.lat, s.lng])
          m.setStyle({ fillColor: color })
        } else {
          const m = L.circleMarker([s.lat, s.lng], {
            radius: 5, fillColor: color, color: '#ffffff', weight: 1.5, fillOpacity: 1,
          })
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
        existing.braking_intensity = c.braking_intensity ?? 0
        existing.st  = c.st
        existing.spd = c.spd
      } else {
        stateRef.current.set(c.id, {
          lat: c.lat, lng: c.lng,
          targetLat: c.lat, targetLng: c.lng,
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
