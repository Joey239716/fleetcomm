import { useState } from 'react'
import './index.css'
import NavBar from './components/NavBar'
import Sidebar from './components/Sidebar'
import FleetMap from './components/FleetMap'
import MapEditor from './components/MapEditor'
import { useFleetSocket } from './hooks/useFleetSocket'
import type { Mode } from './types'

export default function App() {
  const [mode, setMode] = useState<Mode>('human')
  const [showTrail, setShowTrail] = useState(false)
  const [editMap, setEditMap] = useState(false)
  const { frame, connected, sendMode } = useFleetSocket()

  const handleModeChange = (m: Mode) => {
    setMode(m)
    sendMode(m)
  }

  const cars = frame?.cars ?? []
  const tick = frame?.tick ?? 0

  return (
    <div style={{ display: 'flex', flexDirection: 'column', height: '100vh' }}>
      <NavBar
        mode={mode}
        onModeChange={handleModeChange}
        connected={connected}
        tick={tick}
        showTrail={showTrail}
        onToggleTrail={() => setShowTrail(t => !t)}
        editMap={editMap}
        onToggleEditMap={() => setEditMap(e => !e)}
      />
      <div style={{ display: 'flex', height: 'calc(100vh - 52px)', marginTop: 52 }}>
        {editMap ? (
          <main style={{ flex: 1, position: 'relative', minWidth: 0, height: '100%' }}>
            <MapEditor />
          </main>
        ) : (
          <>
            <Sidebar cars={cars} mode={mode} />
            <main style={{ flex: 1, position: 'relative', minWidth: 0, height: '100%' }}>
              <FleetMap cars={cars} mode={mode} showTrail={showTrail} tick={tick} />
            </main>
          </>
        )}
      </div>
    </div>
  )
}
