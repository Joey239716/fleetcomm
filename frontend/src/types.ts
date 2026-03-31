export interface CarState {
  id: number
  lat: number
  lng: number
  spd: number
  hdg: number
  st: number           // 0 = Active, 1 = Dead, 2 = Breakdown
  braking_intensity: number
}

export interface WorldState {
  tick: number
  mode: 'human' | 'v2v'
  cars: CarState[]
}

export type Mode = 'human' | 'v2v'
