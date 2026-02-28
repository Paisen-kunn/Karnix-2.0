import React, { useState, useEffect } from 'react';
import { useAuth } from '../context/AuthContext';
import { patientAPI } from '../services/api';

/* Risk helpers */
const riskColor = (cat) =>
  cat === 'HIGH' ? '#ef4444' : cat === 'MODERATE' ? '#f59e0b' : '#22c55e';
const riskBg = (cat) =>
  cat === 'HIGH' ? '#fef2f2' : cat === 'MODERATE' ? '#fffbeb' : '#f0fdf4';
const riskIcon = (cat) =>
  cat === 'HIGH' ? '🔴' : cat === 'MODERATE' ? '🟡' : '🟢';

/* Score severity label */
const scoreSeverity = (val, threshHigh, threshMod) => {
  if (val >= threshHigh) return { text: 'Severe', color: '#ef4444' };
  if (val >= threshMod)  return { text: 'Moderate', color: '#f59e0b' };
  return { text: 'Normal', color: '#22c55e' };
};

const MetricBar = ({ label, value, max, color, suffix = '' }) => (
  <div className="pp-metric">
    <div className="pp-metric-header">
      <span className="pp-metric-label">{label}</span>
      <span className="pp-metric-value" style={{ color }}>
        {value}{suffix}
      </span>
    </div>
    <div className="pp-bar-track">
      <div
        className="pp-bar-fill"
        style={{ width: `${Math.min(100, (value / max) * 100)}%`, background: color }}
      />
    </div>
  </div>
);

const PatientPortal = () => {
  const { user } = useAuth();
  const [record, setRecord]   = useState(null);
  const [loading, setLoading] = useState(true);
  const [error, setError]     = useState('');

  useEffect(() => {
    loadRecord();
  }, []);

  const loadRecord = async () => {
    try {
      setLoading(true);
      // Try to fetch full record from backend
      const res = await patientAPI.getById(user.patient_id);
      setRecord(res.data.data || res.data);
    } catch {
      // Backend unreachable or no record – use data baked into the JWT
      if (user.patient_id) {
        setRecord({
          patient_id:        user.patient_id,
          age:               user.age,
          sex:               user.sex,
          risk_category:     user.risk_category,
          risk_score:        user.risk_score,
          dl_classification: user.classification,
          scan_date:         user.scan_date,
          cobb_angle:        null,
          bone_density_tscore: null,
          compression_fracture_grade: null,
          disc_degeneration_grade: null,
        });
      } else {
        setError('Could not load your health record. Please contact your clinic.');
      }
    } finally {
      setLoading(false);
    }
  };

  if (loading) return (
    <div className="loading"><div className="spinner" /></div>
  );

  if (error) return (
    <div className="pp-error">
      <div className="pp-error-icon">⚠️</div>
      <p>{error}</p>
    </div>
  );

  if (!record) return null;

  const riskCat   = record.risk_category || 'LOW';
  const riskScore = parseFloat(record.risk_score) || 0;
  const cobbAngle = parseFloat(record.cobb_angle) || 0;
  const boneDensity = parseFloat(record.bone_density_tscore) || 0;
  const fracGrade = record.compression_fracture_grade || 0;
  const discGrade = record.disc_degeneration_grade || 0;
  const classification = record.dl_classification || 'Normal';
  const scanDate  = record.scan_date
    ? new Date(record.scan_date).toLocaleDateString('en-US', {
        year: 'numeric', month: 'long', day: 'numeric'
      })
    : '—';

  return (
    <div className="patient-portal">

      {/* ── Header ────────────────────────────── */}
      <div className="pp-header" style={{ borderColor: riskColor(riskCat) }}>
        <div className="pp-header-left">
          <div className="pp-avatar">{record.sex === 'F' ? '👩' : '🧔'}</div>
          <div>
            <h1 className="pp-patient-id">{record.patient_id}</h1>
            <p className="pp-patient-meta">
              {record.age ? `Age ${record.age}` : ''}
              {record.age && record.sex ? ' · ' : ''}
              {record.sex === 'M' ? 'Male' : record.sex === 'F' ? 'Female' : ''}
            </p>
            <p className="pp-scan-date">Last scan: {scanDate}</p>
          </div>
        </div>

        {/* Risk tier badge */}
        <div
          className="pp-risk-badge"
          style={{ background: riskBg(riskCat), borderColor: riskColor(riskCat) }}
        >
          <div className="pp-risk-icon">{riskIcon(riskCat)}</div>
          <div>
            <div className="pp-risk-label">Risk Level</div>
            <div className="pp-risk-tier" style={{ color: riskColor(riskCat) }}>
              {riskCat}
            </div>
          </div>
        </div>
      </div>

      {/* ── Summary cards ─────────────────────── */}
      <div className="pp-cards">

        <div className="pp-card" style={{ borderTopColor: riskColor(riskCat) }}>
          <div className="pp-card-icon">🎯</div>
          <div className="pp-card-body">
            <div className="pp-card-label">Risk Score</div>
            <div className="pp-card-value" style={{ color: riskColor(riskCat) }}>
              {riskScore.toFixed(0)}
              <span className="pp-card-unit">/ 100</span>
            </div>
            <div className="pp-progress">
              <div
                className="pp-progress-fill"
                style={{ width: `${riskScore}%`, background: riskColor(riskCat) }}
              />
            </div>
          </div>
        </div>

        <div className="pp-card" style={{ borderTopColor: '#6366f1' }}>
          <div className="pp-card-icon">🔬</div>
          <div className="pp-card-body">
            <div className="pp-card-label">AI Diagnosis</div>
            <div
              className="pp-card-value"
              style={{
                color: classification === 'Normal' ? '#22c55e' : '#ef4444',
                fontSize: '1.25rem'
              }}
            >
              {classification}
            </div>
            <div className="pp-card-sub">Deep-learning classification</div>
          </div>
        </div>

        <div className="pp-card" style={{ borderTopColor: '#0ea5e9' }}>
          <div className="pp-card-icon">📅</div>
          <div className="pp-card-body">
            <div className="pp-card-label">Last Scan</div>
            <div className="pp-card-value" style={{ fontSize: '1rem', color: '#1e293b' }}>
              {scanDate}
            </div>
            <div className="pp-card-sub">Spine X-ray analysis</div>
          </div>
        </div>

      </div>

      {/* ── Clinical scores ───────────────────── */}
      <div className="pp-section">
        <h2 className="pp-section-title">📐 Clinical Scores</h2>
        <div className="pp-scores-grid">

          {/* Cobb Angle */}
          <div className="pp-score-card">
            <div className="pp-score-header">
              <span className="pp-score-name">Cobb Angle</span>
              {cobbAngle > 0 && (
                <span
                  className="pp-score-tag"
                  style={{ background: scoreSeverity(cobbAngle, 25, 10).color }}
                >
                  {scoreSeverity(cobbAngle, 25, 10).text}
                </span>
              )}
            </div>
            <div className="pp-score-value">
              {cobbAngle > 0 ? `${cobbAngle.toFixed(1)}°` : '—'}
            </div>
            <div className="pp-score-explanation">
              Measures spinal curvature. Normal &lt;10°, severe &gt;25°.
            </div>
            {cobbAngle > 0 && (
              <MetricBar
                label=""
                value={cobbAngle}
                max={50}
                color={scoreSeverity(cobbAngle, 25, 10).color}
                suffix="°"
              />
            )}
          </div>

          {/* Bone Density */}
          <div className="pp-score-card">
            <div className="pp-score-header">
              <span className="pp-score-name">Bone T-Score</span>
              {boneDensity !== 0 && (
                <span
                  className="pp-score-tag"
                  style={{
                    background:
                      boneDensity < -2.5 ? '#ef4444' :
                      boneDensity < -1   ? '#f59e0b' : '#22c55e'
                  }}
                >
                  {boneDensity < -2.5 ? 'Osteoporosis' : boneDensity < -1 ? 'Low' : 'Normal'}
                </span>
              )}
            </div>
            <div className="pp-score-value">
              {boneDensity !== 0 ? boneDensity.toFixed(1) : '—'}
            </div>
            <div className="pp-score-explanation">
              Bone mineral density. Normal ≥ −1, osteoporosis ≤ −2.5.
            </div>
          </div>

          {/* Fracture Grade */}
          <div className="pp-score-card">
            <div className="pp-score-header">
              <span className="pp-score-name">Fracture Grade</span>
              {fracGrade > 0 && (
                <span
                  className="pp-score-tag"
                  style={{ background: fracGrade >= 2 ? '#ef4444' : '#f59e0b' }}
                >
                  Grade {fracGrade}
                </span>
              )}
            </div>
            <div className="pp-score-value">
              {fracGrade > 0 ? `${fracGrade} / 3` : '0 / 3'}
            </div>
            <div className="pp-score-explanation">
              Vertebral compression fracture severity (0 = none, 3 = severe).
            </div>
            <div className="pp-grade-dots">
              {[1, 2, 3].map(i => (
                <div
                  key={i}
                  className="pp-grade-dot"
                  style={{ background: i <= fracGrade ? '#ef4444' : '#e2e8f0' }}
                />
              ))}
            </div>
          </div>

          {/* Disc Degeneration */}
          <div className="pp-score-card">
            <div className="pp-score-header">
              <span className="pp-score-name">Disc Degeneration</span>
              {discGrade > 0 && (
                <span
                  className="pp-score-tag"
                  style={{ background: discGrade >= 3 ? '#ef4444' : '#f59e0b' }}
                >
                  Grade {discGrade}
                </span>
              )}
            </div>
            <div className="pp-score-value">
              {discGrade > 0 ? `${discGrade} / 4` : '0 / 4'}
            </div>
            <div className="pp-score-explanation">
              Intervertebral disc degeneration scale (0 = healthy, 4 = severe).
            </div>
            <div className="pp-grade-dots">
              {[1, 2, 3, 4].map(i => (
                <div
                  key={i}
                  className="pp-grade-dot"
                  style={{ background: i <= discGrade ? '#f59e0b' : '#e2e8f0' }}
                />
              ))}
            </div>
          </div>

        </div>
      </div>

      {/* ── Recommendations ───────────────────── */}
      <div className="pp-section">
        <h2 className="pp-section-title">💡 Recommendations</h2>
        <div className="pp-reco-list">
          {riskCat === 'HIGH' && (
            <>
              <div className="pp-reco pp-reco-high">
                <span>🚨</span>
                <div>
                  <strong>Urgent follow-up required.</strong> Your risk score is in the HIGH category.
                  Please schedule an appointment with your spine specialist immediately.
                </div>
              </div>
              <div className="pp-reco pp-reco-high">
                <span>🏥</span>
                <div>Consider imaging (MRI/CT) for detailed structural assessment.</div>
              </div>
            </>
          )}
          {riskCat === 'MODERATE' && (
            <>
              <div className="pp-reco pp-reco-mod">
                <span>⚠️</span>
                <div>
                  <strong>Follow-up within 3 months.</strong> Your spine shows moderate abnormalities.
                  Regular physiotherapy and lifestyle changes are advised.
                </div>
              </div>
            </>
          )}
          {riskCat === 'LOW' && (
            <div className="pp-reco pp-reco-low">
              <span>✅</span>
              <div>
                <strong>No urgent action needed.</strong> Your spine health is within normal range.
                Continue regular check-ups every 12 months.
              </div>
            </div>
          )}
          <div className="pp-reco pp-reco-neutral">
            <span>🏃</span>
            <div>
              Maintain regular low-impact exercise (walking, swimming). Avoid heavy lifting without
              proper technique.
            </div>
          </div>
          <div className="pp-reco pp-reco-neutral">
            <span>🥗</span>
            <div>
              Adequate calcium and Vitamin D intake supports bone health. Consider a dietary review
              with your GP.
            </div>
          </div>
        </div>
      </div>

      {/* ── Privacy notice ────────────────────── */}
      <div className="pp-privacy">
        🔒 Your health data is encrypted and accessible only to you and your care team.
        This report was generated from your most recent spine scan.
      </div>

    </div>
  );
};

export default PatientPortal;
