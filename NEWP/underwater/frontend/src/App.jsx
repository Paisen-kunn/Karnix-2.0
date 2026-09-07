import { useState } from 'react'
import UploadPanel from './components/UploadPanel.jsx'
import ResultsPanel from './components/ResultsPanel.jsx'

const API_BASE = ''

export default function App() {
  const [loading, setLoading] = useState(false)
  const [error, setError] = useState(null)
  const [result, setResult] = useState(null)

  async function handleAnalyze({ imageFile, metadataFile, rangeM, minConfidence, detector }) {
    setLoading(true)
    setError(null)
    setResult(null)
    try {
      const form = new FormData()
      form.append('image', imageFile)
      if (metadataFile) form.append('metadata', metadataFile)
      form.append('range_m', String(rangeM))
      form.append('min_confidence', String(minConfidence))
      form.append('detector', detector || 'yolo')

      const res = await fetch(`${API_BASE}/api/analyze`, { method: 'POST', body: form })
      if (!res.ok) {
        const body = await res.json().catch(() => ({}))
        throw new Error(body.description || body.error || `Request failed (${res.status})`)
      }
      const data = await res.json()
      setResult(data)
    } catch (err) {
      setError(err.message || 'Analysis failed')
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="app">
      <header>
        <h1>🌊 AquaSentry – Sonar Anomaly Detection</h1>
        <p>AI-powered detection of marine debris, ghost nets, and shipwrecks in side-scan sonar imagery</p>
      </header>

      {error && <div className="error-banner">⚠️ {error}</div>}

      <UploadPanel onAnalyze={handleAnalyze} loading={loading} />

      {result && <ResultsPanel result={result} apiBase={API_BASE} />}
    </div>
  )
}
