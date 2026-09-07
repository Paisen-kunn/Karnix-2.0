export default function BandGrid({ numBands, activeBands, scannedBand, lastResult }) {
  const resultColor = {
    TP: '#1fbf6b',
    FN: '#e0a800',
    FP: '#e04b4b',
    TN: '#2b2f3a',
  }
  return (
    <div className="band-grid">
      {Array.from({ length: numBands }, (_, b) => {
        const isActive = activeBands.includes(b)
        const isScanned = b === scannedBand
        let className = 'band-cell'
        if (isActive) className += ' band-active'
        if (isScanned) className += ' band-scanned'
        return (
          <div
            key={b}
            className={className}
            style={isScanned ? { borderColor: resultColor[lastResult] || '#fff' } : undefined}
            title={`Band ${b}${isActive ? ' (transmitting)' : ''}${isScanned ? ' (scanned)' : ''}`}
          >
            <span>{b}</span>
          </div>
        )
      })}
    </div>
  )
}
