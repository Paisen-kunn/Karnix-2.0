import { useState } from 'react'

export default function UploadPanel({ onAnalyze, loading }) {
  const [imageFile, setImageFile] = useState(null)
  const [metadataFile, setMetadataFile] = useState(null)
  const [rangeM, setRangeM] = useState(30)
  const [minConfidence, setMinConfidence] = useState(0.35)
  const [detector, setDetector] = useState('yolo')

  function handleSubmit(e) {
    e.preventDefault()
    if (!imageFile) return
    onAnalyze({ imageFile, metadataFile, rangeM, minConfidence, detector })
  }

  return (
    <form className="upload-card" onSubmit={handleSubmit}>
      <div className="detector-select">
        <div>
          <label style={{display: 'inline', fontWeight: '600', marginRight: '20px'}}>
            Detection Model:
          </label>
          <label style={{display: 'inline', marginRight: '20px'}}>
            <input 
              type="radio" 
              value="yolo" 
              checked={detector === 'yolo'}
              onChange={(e) => setDetector(e.target.value)}
            />
            <span style={{marginLeft: '6px', fontWeight: '500'}}>🤖 YOLO (Recommended)</span>
          </label>
          <label style={{display: 'inline'}}>
            <input 
              type="radio" 
              value="classical" 
              checked={detector === 'classical'}
              onChange={(e) => setDetector(e.target.value)}
            />
            <span style={{marginLeft: '6px', fontWeight: '500'}}>📊 Classical CV</span>
          </label>
        </div>
      </div>

      <div className="upload-row">
        <div style={{flex: 1, minWidth: '200px'}}>
          <label>Sonar image *</label>
          <input
            type="file"
            accept=".png,.jpg,.jpeg,.tif,.tiff,.bmp"
            onChange={(e) => setImageFile(e.target.files?.[0] || null)}
          />
        </div>
        <div style={{flex: 1, minWidth: '200px'}}>
          <label>Ping metadata (JSON, optional)</label>
          <input
            type="file"
            accept=".json"
            onChange={(e) => setMetadataFile(e.target.files?.[0] || null)}
          />
        </div>
      </div>
      <div className="upload-row">
        <div style={{flex: 1, minWidth: '150px'}}>
          <label>Sonar range (m)</label>
          <input type="number" value={rangeM} min="1" step="1"
            onChange={(e) => setRangeM(Number(e.target.value))} />
        </div>
        <div style={{flex: 1, minWidth: '150px'}}>
          <label>Min confidence</label>
          <input type="number" value={minConfidence} min="0" max="1" step="0.05"
            onChange={(e) => setMinConfidence(Number(e.target.value))} />
        </div>
      </div>
      <button type="submit" disabled={!imageFile || loading}>
        {loading ? 'Analyzing…' : '🔍 Run Detection'}
      </button>
    </form>
  )
}
