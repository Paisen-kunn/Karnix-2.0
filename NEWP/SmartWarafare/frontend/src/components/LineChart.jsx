export default function LineChart({ data, width = 560, height = 140, color = '#4da3ff', label }) {
  if (!data || data.length === 0) {
    return <div className="chart-empty">No data yet</div>
  }
  const min = Math.min(...data)
  const max = Math.max(...data)
  const range = max - min || 1
  const step = width / Math.max(data.length - 1, 1)
  const points = data
    .map((v, i) => `${(i * step).toFixed(1)},${(height - ((v - min) / range) * height).toFixed(1)}`)
    .join(' ')

  return (
    <div className="chart">
      {label && <div className="chart-label">{label}</div>}
      <svg width={width} height={height} viewBox={`0 0 ${width} ${height}`}>
        <polyline fill="none" stroke={color} strokeWidth="2" points={points} />
      </svg>
    </div>
  )
}
