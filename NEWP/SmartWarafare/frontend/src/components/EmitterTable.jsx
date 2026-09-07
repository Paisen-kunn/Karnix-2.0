function fmt(v, digits = 2) {
  return v === null || v === undefined ? '—' : v.toFixed(digits)
}

export default function EmitterTable({ emitters }) {
  if (!emitters || emitters.length === 0) return null
  return (
    <table className="emitter-table">
      <thead>
        <tr>
          <th>Emitter</th>
          <th>Mode</th>
          <th>Bands</th>
          <th>True Period</th>
          <th>Intercepts</th>
          <th>Avg Intercept Interval</th>
          <th>Intercept Time Error</th>
        </tr>
      </thead>
      <tbody>
        {emitters.map((e) => (
          <tr key={e.id}>
            <td>{e.name}</td>
            <td><span className={`mode-badge mode-${e.mode}`}>{e.mode}</span></td>
            <td>{e.bands ? e.bands.join(', ') : '—'}</td>
            <td>{e.mode === 'periodic' ? e.period : '—'}</td>
            <td>{e.intercepts}</td>
            <td>{fmt(e.avg_intercept_interval)}</td>
            <td>{fmt(e.intercept_time_error)}</td>
          </tr>
        ))}
      </tbody>
    </table>
  )
}
