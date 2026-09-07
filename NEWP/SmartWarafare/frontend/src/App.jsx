import { useCallback, useEffect, useRef, useState } from 'react'
import { api } from './api.js'
import BandGrid from './components/BandGrid.jsx'
import MetricsPanel from './components/MetricsPanel.jsx'
import LineChart from './components/LineChart.jsx'
import EmitterTable from './components/EmitterTable.jsx'

const SCHEDULER_LABELS = {
  round_robin: 'Round Robin (open-loop)',
  random: 'Random (open-loop)',
  epsilon_greedy: 'Epsilon-Greedy (ML)',
  ucb1: 'UCB1 Bandit (ML)',
}

export default function App() {
  const [numBands, setNumBands] = useState(16)
  const [numEmitters, setNumEmitters] = useState(6)
  const [schedulerType, setSchedulerType] = useState('ucb1')
  const [playing, setPlaying] = useState(false)
  const [speed, setSpeed] = useState(5) // steps per tick

  const [emitters, setEmitters] = useState([])
  const [metrics, setMetrics] = useState(null)
  const [lastStep, setLastStep] = useState(null)
  const [rewardHistory, setRewardHistory] = useState([])
  const [hitRateHistory, setHitRateHistory] = useState([])

  const intervalRef = useRef(null)

  const doReset = useCallback(async () => {
    setPlaying(false)
    const res = await api.reset({
      num_bands: Number(numBands),
      num_emitters: Number(numEmitters),
      scheduler_type: schedulerType,
    })
    setEmitters(res.emitters)
    setMetrics(res.metrics)
    setLastStep(null)
    setRewardHistory([])
    setHitRateHistory([])
  }, [numBands, numEmitters, schedulerType])

  useEffect(() => {
    doReset()
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [])

  const doStep = useCallback(async () => {
    const res = await api.step(Number(speed))
    const records = res.steps
    setLastStep(records[records.length - 1])
    setMetrics(res.metrics)
    setRewardHistory((h) => [...h, res.metrics.cumulative_reward].slice(-100))
    setHitRateHistory((h) => [...h, res.metrics.hit_rate * 100].slice(-100))
  }, [speed])

  useEffect(() => {
    if (playing) {
      intervalRef.current = setInterval(doStep, 400)
      return () => clearInterval(intervalRef.current)
    }
  }, [playing, doStep])

  const emittersById = Object.fromEntries(emitters.map((e) => [e.id, e]))
  const mergedEmitters = (metrics?.per_emitter || []).map((pe) => ({
    ...pe,
    bands: emittersById[pe.id]?.bands,
  }))

  return (
    <div className="app">
      <header className="app-header">
        <h1>Smart Scan Strategy for Electronic Warfare</h1>
        <p>ML-based Electronic Support receiver scheduler — simulated RF spectrum demo</p>
      </header>

      <section className="controls">
        <label>
          Bands
          <input type="number" min={4} max={64} value={numBands} onChange={(e) => setNumBands(e.target.value)} />
        </label>
        <label>
          Emitters
          <input type="number" min={1} max={20} value={numEmitters} onChange={(e) => setNumEmitters(e.target.value)} />
        </label>
        <label>
          Scheduler
          <select value={schedulerType} onChange={(e) => setSchedulerType(e.target.value)}>
            {Object.entries(SCHEDULER_LABELS).map(([k, v]) => (
              <option key={k} value={k}>{v}</option>
            ))}
          </select>
        </label>
        <label>
          Steps / tick
          <input type="number" min={1} max={50} value={speed} onChange={(e) => setSpeed(e.target.value)} />
        </label>
        <button onClick={doReset}>Reset</button>
        <button onClick={doStep} disabled={playing}>Step</button>
        <button onClick={() => setPlaying((p) => !p)}>{playing ? 'Pause' : 'Play'}</button>
      </section>

      <section className="board">
        <div className="board-left">
          <h2>Spectrum ({metrics?.t ?? 0} steps)</h2>
          <BandGrid
            numBands={Number(numBands)}
            activeBands={lastStep?.active_bands || []}
            scannedBand={lastStep?.scanned_band}
            lastResult={lastStep?.result}
          />
          <div className="legend">
            <span><i className="dot active" /> Transmitting (ground truth)</span>
            <span><i className="dot scanned" /> Scanned band this step</span>
            <span><i className="dot tp" /> TP <i className="dot fn" /> FN <i className="dot fp" /> FP</span>
          </div>
          <div className="charts">
            <LineChart data={rewardHistory} label="Cumulative Reward" color="#4da3ff" />
            <LineChart data={hitRateHistory} label="Hit Rate % (per tick)" color="#1fbf6b" />
          </div>
        </div>
        <div className="board-right">
          <MetricsPanel metrics={metrics} />
        </div>
      </section>

      <section className="emitters">
        <h2>Emitters (ground truth, for evaluation only)</h2>
        <EmitterTable emitters={mergedEmitters} />
      </section>
    </div>
  )
}
