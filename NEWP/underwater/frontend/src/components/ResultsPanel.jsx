import { useState } from 'react'

function confidenceBadge(score) {
  if (score >= 0.70) return 'high'
  if (score >= 0.40) return 'medium'
  return 'low'
}

function formatLabel(label) {
  if (!label) return 'Marine Debris'
  const map = {
    'shipwreck_plane': '✈️ Airplane Wreck',
    'shipwreck': '🚢 Shipwreck Target',
    'pipe': '⛽ Underwater Pipe',
    'cylinder': '🛢️ Cylinder / Drum',
    'ghost_net': '🕸️ Ghost Net / Rope',
    'debris': '🔍 Marine Anomaly'
  }
  return map[label] || label.replace(/_/g, ' ').toUpperCase()
}

export default function ResultsPanel({ result, apiBase }) {
  const [selectedIdx, setSelectedIdx] = useState(null)
  const [isModalOpen, setIsModalOpen] = useState(false)
  const detections = result.detections || []

  return (
    <div className="results-container">
      <div className="summary-bar">
        <div className="stat">
          <b>{result.detector?.toUpperCase() || 'YOLO'}</b>
          <small>Detection Backend</small>
        </div>
        <div className="stat">
          <b>{result.raw_count}</b>
          <small>Raw Candidates</small>
        </div>
        <div className="stat">
          <b>{result.kept_count}</b>
          <small>Filtered Targets</small>
        </div>
        <div className="stat highlight">
          <b>{detections.length}</b>
          <small>Verified Anomalies</small>
        </div>
      </div>

      <div className="results-grid">
        <div className="overlay-panel">
          <div className="panel-header">
            <h3>Detection Overlay Visualization</h3>
            <button className="expand-btn" onClick={() => setIsModalOpen(true)}>
              🔍 Inspect Fullscreen
            </button>
          </div>
          <div className="overlay-img-wrapper" onClick={() => setIsModalOpen(true)}>
            <img 
              src={`${apiBase}${result.overlay_url}`} 
              alt="Sonar Detection Overlay" 
              className="overlay-img"
            />
          </div>
          <p className="overlay-hint">Click image to expand high-resolution tactical view</p>
        </div>

        <div className="table-panel">
          <div className="panel-header">
            <h3>Target Anomaly Directory</h3>
            <span className="count-badge">{detections.length} Targets</span>
          </div>

          <div className="table-scroll">
            <table>
              <thead>
                <tr>
                  <th>#</th>
                  <th>Target Type</th>
                  <th>Confidence</th>
                  <th>BBox (W×H)</th>
                  <th>Shadow %</th>
                  <th>Lat / Lon</th>
                </tr>
              </thead>
              <tbody>
                {detections.length > 0 ? (
                  detections.map((d, i) => (
                    <tr 
                      key={d.id || i}
                      className={selectedIdx === i ? 'selected-row' : ''}
                      onClick={() => setSelectedIdx(i)}
                    >
                      <td><b>{i + 1}</b></td>
                      <td style={{fontWeight: '600', color: '#0f172a'}}>
                        {formatLabel(d.label || d.class_name)}
                      </td>
                      <td>
                        <span className={`badge ${confidenceBadge(d.confidence)}`}>
                          {Math.round((d.confidence || 0) * 100)}%
                        </span>
                      </td>
                      <td>
                        <span className="dim-text">
                          {d.bbox_px ? `${d.bbox_px.w}×${d.bbox_px.h} px` : '—'}
                        </span>
                      </td>
                      <td>
                        {d.features?.shadow_ratio != null ? `${Math.round(d.features.shadow_ratio * 100)}%` : '—'}
                      </td>
                      <td>
                        {d.latitude != null ? (
                          <span className="geo-text">
                            {d.latitude.toFixed(4)}, {d.longitude.toFixed(4)}
                          </span>
                        ) : (
                          <span className="dim-text">Across-track</span>
                        )}
                      </td>
                    </tr>
                  ))
                ) : (
                  <tr>
                    <td colSpan="6" className="empty-state">
                      No marine anomalies detected at current confidence threshold.
                    </td>
                  </tr>
                )}
              </tbody>
            </table>
          </div>

          <div className="downloads">
            <a href={`${apiBase}${result.report_json_url}`} download className="download-btn json">
              📄 Export JSON Report
            </a>
            <a href={`${apiBase}${result.report_csv_url}`} download className="download-btn csv">
              📊 Export CSV Report
            </a>
          </div>
        </div>
      </div>

      {isModalOpen && (
        <div className="img-modal-backdrop" onClick={() => setIsModalOpen(false)}>
          <div className="img-modal-content" onClick={(e) => e.stopPropagation()}>
            <div className="modal-header">
              <h4>Tactical High-Resolution Overlay Inspection</h4>
              <button className="close-btn" onClick={() => setIsModalOpen(false)}>✕</button>
            </div>
            <img 
              src={`${apiBase}${result.overlay_url}`} 
              alt="Sonar Overlay Fullscreen" 
              className="modal-img"
            />
          </div>
        </div>
      )}
    </div>
  )
}
