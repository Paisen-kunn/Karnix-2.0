function fmtPct(v) {
  return v === null || v === undefined ? '—' : `${(v * 100).toFixed(1)}%`
}
function fmtNum(v, digits = 2) {
  return v === null || v === undefined ? '—' : v.toFixed(digits)
}

export default function MetricsPanel({ metrics }) {
  if (!metrics) return null
  const cards = [
    { label: 'Probability of Detection (Pd)', value: fmtPct(metrics.pd) },
    { label: 'Probability of False Alarm (Pfa)', value: fmtPct(metrics.pfa) },
    { label: 'Sensitivity', value: fmtPct(metrics.sensitivity) },
    { label: 'Hit Rate / Avg Intercept Rate', value: fmtPct(metrics.hit_rate) },
    { label: 'Interception Ratio', value: fmtPct(metrics.interception_ratio) },
    { label: '% Correct Predictions', value: `${metrics.percent_correct_predictions.toFixed(1)}%` },
    { label: 'Avg Reward / Step', value: fmtNum(metrics.avg_reward, 3) },
    { label: 'Cumulative Reward', value: fmtNum(metrics.cumulative_reward, 1) },
    { label: 'Avg Intercept Time (steps)', value: fmtNum(metrics.avg_intercept_time, 2) },
    { label: 'Avg Intercept Time Error', value: fmtNum(metrics.avg_intercept_time_error, 2) },
  ]
  return (
    <div className="metrics-panel">
      {cards.map((c) => (
        <div key={c.label} className="metric-card">
          <div className="metric-value">{c.value}</div>
          <div className="metric-label">{c.label}</div>
        </div>
      ))}
    </div>
  )
}
