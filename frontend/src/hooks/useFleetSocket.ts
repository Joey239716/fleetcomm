import { useEffect, useRef, useState, useCallback } from 'react'
import type { WorldState, Mode } from '../types'

const WS_URL = import.meta.env.VITE_WS_URL ?? 'ws://localhost:8080/ws'

export function useFleetSocket() {
  const wsRef = useRef<WebSocket | null>(null)
  const [frame, setFrame] = useState<WorldState | null>(null)
  const [connected, setConnected] = useState(false)

  useEffect(() => {
    const ws = new WebSocket(WS_URL)
    wsRef.current = ws

    ws.onopen  = () => setConnected(true)
    ws.onclose = () => setConnected(false)
    ws.onmessage = (e: MessageEvent) => {
      setFrame(JSON.parse(e.data) as WorldState)
    }

    return () => ws.close()
  }, [])

  const sendMode = useCallback((mode: Mode) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify({ mode }))
    }
  }, [])

  return { frame, connected, sendMode }
}
