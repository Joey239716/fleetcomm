import type { CarState, Mode } from '../types'

interface Props {
  cars: CarState[]
  mode: Mode
}

function brakingColor(intensity: number): string {
  // 0 = full speed (green), 1 = stopped (red)
  const r = Math.round(45  + (224 - 45)  * intensity)
  const g = Math.round(184 + (58  - 184) * intensity)
  const b = Math.round(75  + (58  - 75)  * intensity)
  return `rgb(${r},${g},${b})`
}

export default function Sidebar({ cars, mode }: Props) {
  const active    = cars.filter(c => c.st === 0)
  const breakdown = cars.filter(c => c.st === 2)
  const stopped   = active.filter(c => c.spd < 5)
  // throughput = avg(speed / speedLimit) across active cars; braking_intensity = 1 - speed/limit
  const avgThroughput = active.length > 0
    ? active.reduce((s, c) => s + (1 - (c.braking_intensity ?? 0)), 0) / active.length
    : null

  const accentColor = mode === 'human' ? '#FF6B35' : '#2DB84B'

  const slabel: React.CSSProperties = {
    fontSize: 9, fontWeight: 700, textTransform: 'uppercase',
    letterSpacing: '0.16em', color: '#6e6e73',
  }

  return (
    <aside style={{
      width: 228, flexShrink: 0,
      display: 'flex', flexDirection: 'column',
      background: 'rgba(255,255,255,0.88)',
      backdropFilter: 'blur(24px)',
      borderRight: '1px solid rgba(0,0,0,0.08)',
      zIndex: 40,
    }}>

      {/* Live Metrics */}
      <div style={{ padding: 14, borderBottom: '1px solid rgba(0,0,0,0.07)' }}>
        <div style={{ ...slabel, marginBottom: 12 }}>Live Metrics</div>
        <div style={{ display: 'flex', flexDirection: 'column', gap: 10 }}>
          <div>
            <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: 11, marginBottom: 5 }}>
              <span style={{ color: '#48484a' }}>Throughput</span>
              <span style={{ fontFamily: "'JetBrains Mono', monospace", color: accentColor }}>
                {avgThroughput !== null ? `${(avgThroughput * 100).toFixed(1)}%` : '--'}
              </span>
            </div>
            <div style={{ height: 2, background: '#e8e5e0', borderRadius: 1, overflow: 'hidden' }}>
              <div style={{
                height: '100%',
                background: `linear-gradient(90deg, ${mode === 'human' ? '#e85a1d,#FF6B35' : '#1db84b,#2DB84B'})`,
                borderRadius: 1,
                transition: 'width 0.5s ease',
                width: avgThroughput !== null ? `${Math.min(100, avgThroughput * 100)}%` : '0%',
              }} />
            </div>
          </div>
          <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: 11 }}>
            <span style={{ color: '#48484a' }}>Active</span>
            <span style={{ fontFamily: "'JetBrains Mono', monospace", color: '#2DB84B' }}>{active.length || '--'}</span>
          </div>
          <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: 11 }}>
            <span style={{ color: '#48484a' }}>Breakdown</span>
            <span style={{ fontFamily: "'JetBrains Mono', monospace", color: '#E03A3A' }}>{breakdown.length || '--'}</span>
          </div>
          <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: 11 }}>
            <span style={{ color: '#48484a' }}>Stopped</span>
            <span style={{ fontFamily: "'JetBrains Mono', monospace", color: '#6e6e73' }}>{stopped.length || '--'}</span>
          </div>
        </div>
      </div>

      {/* Fleet Roster */}
      <div style={{ flex: 1, padding: 14, overflow: 'hidden', display: 'flex', flexDirection: 'column', minHeight: 0 }}>
        <div style={{ ...slabel, marginBottom: 10 }}>Fleet Roster</div>
        <div style={{ flex: 1, overflowY: 'auto', display: 'flex', flexDirection: 'column', gap: 2 }}>
          {[...cars].sort((a, b) => a.id - b.id).map(car => {
            const isBreakdown = car.st === 2
            const isDead      = car.st === 1
            const dotColor    = isBreakdown ? '#E03A3A' : isDead ? '#8e8e93' : brakingColor(car.braking_intensity ?? 0)
            const label       = isBreakdown ? 'BREAKDOWN' : isDead ? 'CLEARED' : `${Math.round(car.spd)} km/h`
            const textColor   = isBreakdown ? '#E03A3A' : isDead ? '#6e6e73' : '#1c1c1e'

            return (
              <div key={car.id} style={{ display: 'flex', alignItems: 'center', gap: 8, padding: '4px 6px', borderRadius: 4 }}>
                <div style={{ width: 6, height: 6, borderRadius: '50%', background: dotColor, boxShadow: `0 0 4px ${dotColor}`, flexShrink: 0 }} />
                <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 10, color: textColor, flex: 1 }}>
                  Car {car.id}
                </span>
                <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 9, color: dotColor }}>
                  {label}
                </span>
              </div>
            )
          })}
        </div>
      </div>
    </aside>
  )
}
