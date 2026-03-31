import type { Mode } from '../types'

interface Props {
  mode: Mode
  onModeChange: (m: Mode) => void
  connected: boolean
  tick: number
  showTrail: boolean
  onToggleTrail: () => void
  editMap: boolean
  onToggleEditMap: () => void
}

export default function NavBar({ mode, onModeChange, connected, tick, showTrail, onToggleTrail, editMap, onToggleEditMap }: Props) {
  const btnBase: React.CSSProperties = {
    padding: '5px 14px',
    fontSize: 10,
    fontWeight: 700,
    textTransform: 'uppercase',
    letterSpacing: '0.1em',
    borderRadius: 4,
    fontFamily: "'JetBrains Mono', monospace",
    transition: 'all 0.2s',
    border: '1px solid',
    background: 'transparent',
    cursor: 'pointer',
  }

  const humanActive = mode === 'human'

  return (
    <nav style={{
      position: 'fixed', top: 0, left: 0, right: 0, height: 52,
      display: 'flex', alignItems: 'center', justifyContent: 'space-between',
      padding: '0 20px',
      background: 'rgba(255,255,255,0.94)',
      backdropFilter: 'blur(20px)',
      borderBottom: '1px solid rgba(0,0,0,0.09)',
      zIndex: 100,
      boxShadow: '0 1px 8px rgba(0,0,0,0.06)',
    }}>
      <div style={{ display: 'flex', alignItems: 'center', gap: 14 }}>
        <span style={{ fontFamily: "'Space Grotesk', sans-serif", fontSize: 19, fontWeight: 700, color: '#FF6B35', letterSpacing: '-0.3px' }}>
          FleetComm
        </span>
        <div style={{ display: 'flex', alignItems: 'center', gap: 7 }}>
          <div
            className="blink"
            style={{ width: 5, height: 5, borderRadius: '50%', background: connected ? '#2DB84B' : '#E03A3A', boxShadow: `0 0 6px ${connected ? 'rgba(45,184,75,0.5)' : 'rgba(224,58,58,0.5)'}` }}
          />
          <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 9, color: '#6e6e73', textTransform: 'uppercase', letterSpacing: '0.12em' }}>
            {connected ? `Tick: ${tick}` : 'Connecting...'}
          </span>
        </div>
      </div>

      <div style={{ display: 'flex', alignItems: 'center', gap: 6 }}>
        <button
          onClick={() => onModeChange('human')}
          style={{
            ...btnBase,
            borderColor: humanActive ? '#FF6B35' : 'rgba(0,0,0,0.15)',
            color: humanActive ? '#FF6B35' : '#6e6e73',
            background: humanActive ? 'rgba(255,107,53,0.08)' : 'transparent',
          }}
        >
          Human
        </button>
        <button
          onClick={() => onModeChange('v2v')}
          style={{
            ...btnBase,
            borderColor: !humanActive ? '#2DB84B' : 'rgba(0,0,0,0.15)',
            color: !humanActive ? '#2DB84B' : '#6e6e73',
            background: !humanActive ? 'rgba(45,184,75,0.08)' : 'transparent',
          }}
        >
          V2V
        </button>
        <button
          onClick={onToggleTrail}
          style={{
            ...btnBase,
            borderColor: showTrail ? '#a855f7' : 'rgba(0,0,0,0.15)',
            color: showTrail ? '#a855f7' : '#6e6e73',
            background: showTrail ? 'rgba(168,85,247,0.08)' : 'transparent',
          }}
        >
          Trail
        </button>
        <div style={{ width: 1, height: 22, background: 'rgba(0,0,0,0.12)', margin: '0 4px' }} />
        <button
          onClick={onToggleEditMap}
          style={{
            ...btnBase,
            borderColor: editMap ? '#f97316' : 'rgba(0,0,0,0.15)',
            color: editMap ? '#f97316' : '#94a3b8',
            background: editMap ? 'rgba(249,115,22,0.08)' : 'transparent',
          }}
        >
          Edit Map
        </button>
      </div>
    </nav>
  )
}
