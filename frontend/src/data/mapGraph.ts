export type ControlType = 'TrafficLight' | 'StopSign'

export interface MapNode {
  id: string
  name: string
  lat: number
  lng: number
  control: ControlType
}

export interface MapEdge {
  id: string
  road: string
  from: string
  to: string
  oneWay: boolean
  speedLimit: number
}

export const NODES: MapNode[] = [
  { id: 'N1',  name: 'Jackson & Kearny',        lat: 37.796226, lng: -122.405143, control: 'StopSign' },
  { id: 'N2',  name: 'Jackson & Montgomery',    lat: 37.796424, lng: -122.403520, control: 'StopSign' },
  { id: 'N3',  name: 'Jackson & Sansome',       lat: 37.796630, lng: -122.401878, control: 'StopSign' },
  { id: 'N4',  name: 'Jackson & Battery',       lat: 37.796780, lng: -122.400702, control: 'StopSign' },
  { id: 'N5',  name: 'Jackson & Front',         lat: 37.796940, lng: -122.399511, control: 'StopSign' },
  { id: 'N6',  name: 'Clay & Kearny',           lat: 37.794466, lng: -122.404791, control: 'StopSign' },
  { id: 'N7',  name: 'Clay & Montgomery',       lat: 37.794680, lng: -122.403154, control: 'StopSign' },
  { id: 'N8',  name: 'Clay & Sansome',          lat: 37.794857, lng: -122.401507, control: 'StopSign' },
  { id: 'N9',  name: 'Clay & Battery',          lat: 37.794998, lng: -122.400335, control: 'StopSign' },
  { id: 'N10', name: 'Clay & Front',            lat: 37.795198, lng: -122.399181, control: 'StopSign' },
  { id: 'N11', name: 'Sacramento & Kearny',     lat: 37.793578, lng: -122.404616, control: 'StopSign' },
  { id: 'N12', name: 'Sacramento & Montgomery', lat: 37.793790, lng: -122.402972, control: 'StopSign' },
  { id: 'N13', name: 'Sacramento & Sansome',    lat: 37.793995, lng: -122.401333, control: 'StopSign' },
  { id: 'N14', name: 'Sacramento & Battery',    lat: 37.794143, lng: -122.400158, control: 'StopSign' },
  { id: 'N15', name: 'Sacramento & Front',      lat: 37.794291, lng: -122.398966, control: 'StopSign' },
  { id: 'N16', name: 'California & Kearny',     lat: 37.792657, lng: -122.404423, control: 'TrafficLight' },
  { id: 'N17', name: 'California & Montgomery', lat: 37.792861, lng: -122.402786, control: 'TrafficLight' },
  { id: 'N18', name: 'California & Sansome',    lat: 37.793065, lng: -122.401145, control: 'TrafficLight' },
  { id: 'N19', name: 'California & Battery',    lat: 37.793215, lng: -122.399973, control: 'TrafficLight' },
  { id: 'N20', name: 'California & Front',      lat: 37.793370, lng: -122.398798, control: 'TrafficLight' },
  { id: 'N21', name: 'Pine & Kearny',           lat: 37.791700, lng: -122.404236, control: 'StopSign' },
  { id: 'N22', name: 'Pine & Montgomery',       lat: 37.791911, lng: -122.402597, control: 'StopSign' },
  { id: 'N23', name: 'Pine & Sansome',          lat: 37.792117, lng: -122.400958, control: 'StopSign' },
  { id: 'N24', name: 'Pine & Battery',          lat: 37.792266, lng: -122.399782, control: 'StopSign' },
  { id: 'N25', name: 'Pine & Front',            lat: 37.792413, lng: -122.398610, control: 'StopSign' },
  { id: 'N26', name: 'Bush & Kearny',           lat: 37.790763, lng: -122.404054, control: 'StopSign' },
  { id: 'N27', name: 'Bush & Montgomery',       lat: 37.790981, lng: -122.402409, control: 'StopSign' },
  { id: 'N28', name: 'Bush & Sansome',          lat: 37.791181, lng: -122.400762, control: 'StopSign' },
  { id: 'N29', name: 'Bush & Battery',          lat: 37.791335, lng: -122.399605, control: 'StopSign' },
  { id: 'N31', name: 'Market & Kearny',         lat: 37.787687, lng: -122.403432, control: 'TrafficLight' },
  { id: 'N32', name: 'Market & Montgomery',     lat: 37.788815, lng: -122.401983, control: 'TrafficLight' },
  { id: 'N30', name: 'Bush & Market',           lat: 37.789943, lng: -122.400700, control: 'TrafficLight' },
  { id: 'N33', name: 'Front & Market',          lat: 37.790900, lng: -122.398900, control: 'TrafficLight' },
]

export const EDGES: MapEdge[] = [
  { id: 'E1',  road: 'Jackson St',    from: 'N1',  to: 'N2',  oneWay: false, speedLimit: 40 },
  { id: 'E2',  road: 'Jackson St',    from: 'N2',  to: 'N3',  oneWay: false, speedLimit: 40 },
  { id: 'E3',  road: 'Jackson St',    from: 'N3',  to: 'N4',  oneWay: false, speedLimit: 40 },
  { id: 'E4',  road: 'Jackson St',    from: 'N4',  to: 'N5',  oneWay: false, speedLimit: 40 },
  { id: 'E5',  road: 'Clay St',       from: 'N6',  to: 'N7',  oneWay: true,  speedLimit: 40 },
  { id: 'E6',  road: 'Clay St',       from: 'N7',  to: 'N8',  oneWay: true,  speedLimit: 40 },
  { id: 'E7',  road: 'Clay St',       from: 'N8',  to: 'N9',  oneWay: true,  speedLimit: 40 },
  { id: 'E8',  road: 'Clay St',       from: 'N9',  to: 'N10', oneWay: true,  speedLimit: 40 },
  { id: 'E9',  road: 'Sacramento St', from: 'N15', to: 'N14', oneWay: true,  speedLimit: 40 },
  { id: 'E10', road: 'Sacramento St', from: 'N14', to: 'N13', oneWay: true,  speedLimit: 40 },
  { id: 'E11', road: 'Sacramento St', from: 'N13', to: 'N12', oneWay: true,  speedLimit: 40 },
  { id: 'E12', road: 'Sacramento St', from: 'N12', to: 'N11', oneWay: true,  speedLimit: 40 },
  { id: 'E13', road: 'California St', from: 'N16', to: 'N17', oneWay: false, speedLimit: 48 },
  { id: 'E14', road: 'California St', from: 'N17', to: 'N18', oneWay: false, speedLimit: 48 },
  { id: 'E15', road: 'California St', from: 'N18', to: 'N19', oneWay: false, speedLimit: 48 },
  { id: 'E16', road: 'California St', from: 'N19', to: 'N20', oneWay: false, speedLimit: 48 },
  { id: 'E17', road: 'Pine St',       from: 'N25', to: 'N24', oneWay: true,  speedLimit: 40 },
  { id: 'E18', road: 'Pine St',       from: 'N24', to: 'N23', oneWay: true,  speedLimit: 40 },
  { id: 'E19', road: 'Pine St',       from: 'N23', to: 'N22', oneWay: true,  speedLimit: 40 },
  { id: 'E20', road: 'Pine St',       from: 'N22', to: 'N21', oneWay: true,  speedLimit: 40 },
  { id: 'E21', road: 'Bush St',       from: 'N26', to: 'N27', oneWay: true,  speedLimit: 40 },
  { id: 'E22', road: 'Bush St',       from: 'N27', to: 'N28', oneWay: true,  speedLimit: 40 },
  { id: 'E23', road: 'Bush St',       from: 'N28', to: 'N29', oneWay: true,  speedLimit: 40 },
  { id: 'E25', road: 'Market St',     from: 'N31', to: 'N32', oneWay: false, speedLimit: 48 },
  { id: 'E54', road: 'Market St',     from: 'N32', to: 'N30', oneWay: false, speedLimit: 48 },
  { id: 'E55', road: 'Market St',     from: 'N30', to: 'N33', oneWay: false, speedLimit: 48 },
  { id: 'E53', road: 'Battery St',    from: 'N29', to: 'N30', oneWay: true,  speedLimit: 40 },
  { id: 'E56', road: 'Front St',      from: 'N33', to: 'N25', oneWay: true,  speedLimit: 40 },
  { id: 'E26', road: 'Kearny St',     from: 'N1',  to: 'N6',  oneWay: true,  speedLimit: 40 },
  { id: 'E27', road: 'Kearny St',     from: 'N6',  to: 'N11', oneWay: true,  speedLimit: 40 },
  { id: 'E28', road: 'Kearny St',     from: 'N11', to: 'N16', oneWay: true,  speedLimit: 40 },
  { id: 'E29', road: 'Kearny St',     from: 'N16', to: 'N21', oneWay: true,  speedLimit: 40 },
  { id: 'E30', road: 'Kearny St',     from: 'N21', to: 'N26', oneWay: true,  speedLimit: 40 },
  { id: 'E31', road: 'Kearny St',     from: 'N26', to: 'N31', oneWay: true,  speedLimit: 40 },
  { id: 'E32', road: 'Montgomery St', from: 'N2',  to: 'N7',  oneWay: false, speedLimit: 48 },
  { id: 'E33', road: 'Montgomery St', from: 'N7',  to: 'N12', oneWay: false, speedLimit: 48 },
  { id: 'E34', road: 'Montgomery St', from: 'N12', to: 'N17', oneWay: false, speedLimit: 48 },
  { id: 'E35', road: 'Montgomery St', from: 'N17', to: 'N22', oneWay: false, speedLimit: 48 },
  { id: 'E36', road: 'Montgomery St', from: 'N22', to: 'N27', oneWay: false, speedLimit: 48 },
  { id: 'E37', road: 'Montgomery St', from: 'N27', to: 'N32', oneWay: false, speedLimit: 48 },
  { id: 'E38', road: 'Sansome St',    from: 'N28', to: 'N23', oneWay: true,  speedLimit: 40 },
  { id: 'E39', road: 'Sansome St',    from: 'N23', to: 'N18', oneWay: true,  speedLimit: 40 },
  { id: 'E40', road: 'Sansome St',    from: 'N18', to: 'N13', oneWay: true,  speedLimit: 40 },
  { id: 'E41', road: 'Sansome St',    from: 'N13', to: 'N8',  oneWay: true,  speedLimit: 40 },
  { id: 'E42', road: 'Sansome St',    from: 'N8',  to: 'N3',  oneWay: true,  speedLimit: 40 },
  { id: 'E43', road: 'Battery St',    from: 'N4',  to: 'N9',  oneWay: true,  speedLimit: 40 },
  { id: 'E44', road: 'Battery St',    from: 'N9',  to: 'N14', oneWay: true,  speedLimit: 40 },
  { id: 'E45', road: 'Battery St',    from: 'N14', to: 'N19', oneWay: true,  speedLimit: 40 },
  { id: 'E46', road: 'Battery St',    from: 'N19', to: 'N24', oneWay: true,  speedLimit: 40 },
  { id: 'E47', road: 'Battery St',    from: 'N24', to: 'N29', oneWay: true,  speedLimit: 40 },
  { id: 'E49', road: 'Front St',      from: 'N25', to: 'N20', oneWay: true,  speedLimit: 40 },
  { id: 'E50', road: 'Front St',      from: 'N20', to: 'N15', oneWay: true,  speedLimit: 40 },
  { id: 'E51', road: 'Front St',      from: 'N15', to: 'N10', oneWay: true,  speedLimit: 40 },
  { id: 'E52', road: 'Front St',      from: 'N10', to: 'N5',  oneWay: true,  speedLimit: 40 },
]
