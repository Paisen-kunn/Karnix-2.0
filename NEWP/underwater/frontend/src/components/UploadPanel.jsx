import { useState } from 'react'

export default function UploadPanel({ onAnalyze, loading }) {
  const [imageFile, setImageFile] = useState(null)
  const [previewUrl, setPreviewUrl] = useState(null)
  const [metadataFile, setMetadataFile] = useState(null)
  const [rangeM, setRangeM] = useState(30)
  const [minConfidence, setMinConfidence] = useState(0.35)
  const [detector, setDetector] = useState('yolo')

  function handleImageChange(e) {
    const file = e.target.files?.[0] || null
    setImageFile(file)
    if (file) {
      const url = URL.createObjectURL(file)
      setPreviewUrl(url)
    } else {
      setPreviewUrl(null)
    }
  }

  function handleSubmit(e) {
    e.preventDefault()
    if (!imageFile) return
    onAnalyze({ imageFile, metadataFile, rangeM, minConfidence, detector })
  }

  return (
    <form className="upload-card" onSubmit={handleSubmit}>
      <div className="detector-card-group">
        <label className="section-label">Detection Algorithm Backend:</label>
        <div className="detector-options">
          <label className={`detector-pill ${detector === 'yolo' ? 'active' : ''}`}>
            <input 
              type="radio" 
              name="detector"
              value="yolo" 
              checked={detector === 'yolo'}
              onChange={(e) => setDetector(e.target.value)}
            />
            <div className="pill-content">
              <span className="pill-title">🤖 YOLO AI Detector (Recommended)</span>
              <span className="pill-desc">Ultralytics YOLOv8 + Acoustic Saliency Physics Fusion</span>
            </div>
          </label>

          <label className={`detector-pill ${detector === 'classical' ? 'active' : ''}`}>
            <input 
              type="radio" 
              name="detector"
              value="classical" 
              checked={detector === 'classical'}
              onChange={(e) => setDetector(e.target.value)}
            />
            <div className="pill-content">
              <span className="pill-title">📊 Classical CV Detector</span>
              <span className="pill-desc">Adaptive Thresholding + Acoustic Shadow Analysis</span>
            </div>
          </label>
        </div>
      </div>

      <div className="upload-row main-files">
        <div className="file-input-box">
          <label>Sonar Image Scan *</label>
          <input
            type="file"
            accept=".png,.jpg,.jpeg,.tif,.tiff,.bmp"
            onChange={handleImageChange}
          />
          {previewUrl && (
            <div className="image-preview-container">
              <img src={previewUrl} alt="Sonar upload preview" className="image-preview-thumb" />
              <span className="preview-filename">{imageFile?.name}</span>
            </div>
          )}
        </div>

        <div className="file-input-box">
          <label>Ping Metadata (JSON, optional)</label>
          <input
            type="file"
            accept=".json"
            onChange={(e) => setMetadataFile(e.target.files?.[0] || null)}
          />
          <span className="input-hint">For geotagging lat/lon coordinates along track</span>
        </div>
      </div>

      <div className="upload-row params">
        <div className="param-box">
          <label>Sonar Range (m)</label>
          <input type="number" value={rangeM} min="1" step="1"
            onChange={(e) => setRangeM(Number(e.target.value))} />
        </div>

        <div className="param-box">
          <label>Min Confidence Threshold ({Math.round(minConfidence * 100)}%)</label>
          <input type="range" value={minConfidence} min="0.10" max="0.90" step="0.05"
            onChange={(e) => setMinConfidence(Number(e.target.value))} />
        </div>
      </div>

      <button type="submit" className="analyze-btn" disabled={!imageFile || loading}>
        {loading ? '⚡ Running Detection Pipeline…' : '🔍 Run Sonar Anomaly Detection'}
      </button>
    </form>
  )
}
