function confidenceBadge(score) {
  if (score >= 0.7) return 'high'
  if (score >= 0.4) return 'medium'
  return 'low'
}

export default function ResultsPanel({ result, apiBase }) {
  const detections = result.detections || []

  return (
    <div>
      <div className="summary-bar">
        <div className="stat">
          <b>{result.raw_count}</b>
          <small>Raw Detections</small>
        </div>
        <div className="stat">
          <b>{result.kept_count}</b>
          <small>After Filtering</small>
        </div>
        <div className="stat">
          <b>{detections.length}</b>
          <small>Reported Anomalies</small>
        </div>
      </div>

      <div className="results-grid">
        <div className="overlay-panel">
          <h3 style={{marginTop: 0, color: '#1a202c', marginBottom: '12px'}}>Detection Overlay</h3>
          <img src={`${apiBase}${result.overlay_url}`} alt="Detection overlay" />
        </div>

        <div className="table-panel">
          <h3 style={{marginTop: 0, color: '#1a202c', marginBottom: '16px'}}>Detected Objects</h3>
          <table>
            <thead>
              <tr>
                <th>#</th>
                <th>Class</th>
                <th>Confidence</th>
                <th>Lat</th>
                <th>Lon</th>
              </tr>
            </thead>
            <tbody>
              {detections.length > 0 ? (
                detections.map((d, i) => (
                  <tr key={d.id || i}>
                    <td>{i + 1}</td>
                    <td style={{fontWeight: '500'}}>{d.class_name || d.label}</td>
                    <td>
                      <span className={`badge ${confidenceBadge(d.confidence)}`}>
                        {Math.round((d.confidence || 0) * 100)}%
                      </span>
                    </td>
                    <td>{d.latitude?.toFixed?.(5) ?? d.lat ?? '—'}</td>
                    <td>{d.longitude?.toFixed?.(5) ?? d.lon ?? '—'}</td>
                  </tr>
                ))
              ) : (
                <tr>
                  <td colSpan="5" style={{textAlign: 'center', color: '#94a3b8', padding: '20px'}}>
                    No anomalies detected
                  </td>
                </tr>
              )}
            </tbody>
          </table>

          <div className="downloads">
            <a href={`${apiBase}${result.report_json_url}`} download>📄 JSON Report</a>
            <a href={`${apiBase}${result.report_csv_url}`} download>📊 CSV Report</a>
          </div>
        </div>
      </div>
    </div>
  )
}
