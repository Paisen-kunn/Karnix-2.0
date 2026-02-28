import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { useAuth } from '../context/AuthContext';

const Login = () => {
  const [tab, setTab] = useState('patient'); // 'patient' | 'staff'
  const [patientId, setPatientId] = useState('');
  const [staffCreds, setStaffCreds] = useState({ username: '', password: '' });
  const [error, setError] = useState('');
  const [loading, setLoading] = useState(false);
  const navigate = useNavigate();
  const { login, patientLogin } = useAuth();

  /* ─── Patient Login ─────────────────────────── */
  const handlePatientSubmit = async (e) => {
    e.preventDefault();
    if (!patientId.trim()) { setError('Please enter your Patient ID'); return; }
    setError(''); setLoading(true);
    const result = await patientLogin(patientId.trim());
    setLoading(false);
    if (result.success) navigate('/my-portal');
    else setError(result.error);
  };

  /* ─── Staff Login ───────────────────────────── */
  const handleStaffSubmit = async (e) => {
    e.preventDefault();
    setError(''); setLoading(true);
    const result = await login(staffCreds);
    setLoading(false);
    if (result.success) navigate('/dashboard');
    else setError(result.error);
  };

  return (
    <div className="login-page">
      {/* Left panel – branding */}
      <div className="login-branding">
        <div className="login-branding-inner">
          <div className="login-logo">🦴</div>
          <h1 className="login-title">Spine Analytics</h1>
          <p className="login-subtitle">
            AI-powered spinal health management platform
          </p>
          <div className="login-features">
            <div className="login-feature"><span>🎯</span> Real-time risk scoring</div>
            <div className="login-feature"><span>📊</span> Clinical data analytics</div>
            <div className="login-feature"><span>🔬</span> Deep-learning X-ray analysis</div>
            <div className="login-feature"><span>🛡️</span> Secure &amp; HIPAA-compliant</div>
          </div>
        </div>
      </div>

      {/* Right panel – login card */}
      <div className="login-form-panel">
        <div className="login-card">
          <h2 className="login-card-title">Welcome back</h2>
          <p className="login-card-sub">Sign in to your account</p>

          {/* Tab switcher */}
          <div className="login-tabs">
            <button
              className={`login-tab${tab === 'patient' ? ' active' : ''}`}
              onClick={() => { setTab('patient'); setError(''); }}
            >
              🧑‍⚕️ Patient
            </button>
            <button
              className={`login-tab${tab === 'staff' ? ' active' : ''}`}
              onClick={() => { setTab('staff'); setError(''); }}
            >
              👨‍💼 Staff / Admin
            </button>
          </div>

          {error && (
            <div className="login-error">
              <span>⚠️</span> {error}
            </div>
          )}

          {/* ── Patient form ── */}
          {tab === 'patient' && (
            <form onSubmit={handlePatientSubmit} className="login-form">
              <p className="login-form-desc">
                Enter the Patient ID provided by your clinic to access your personal health records.
              </p>
              <div className="form-group">
                <label htmlFor="patient_id">Patient ID</label>
                <div className="input-icon-wrap">
                  <span className="input-icon">🆔</span>
                  <input
                    type="text"
                    id="patient_id"
                    className="form-control with-icon"
                    placeholder="e.g. PT-001"
                    value={patientId}
                    onChange={e => setPatientId(e.target.value.toUpperCase())}
                    autoFocus
                    required
                  />
                </div>
              </div>
              <button type="submit" className="btn-login" disabled={loading}>
                {loading ? <span className="btn-spinner" /> : '🔓 Access My Records'}
              </button>
              <div className="login-hint">
                <strong>Demo patient IDs:</strong> PT-001 through PT-020
              </div>
            </form>
          )}

          {/* ── Staff form ── */}
          {tab === 'staff' && (
            <form onSubmit={handleStaffSubmit} className="login-form">
              <p className="login-form-desc">
                Staff and administrators can access full analytics and patient management.
              </p>
              <div className="form-group">
                <label htmlFor="username">Username</label>
                <div className="input-icon-wrap">
                  <span className="input-icon">👤</span>
                  <input
                    type="text"
                    id="username"
                    className="form-control with-icon"
                    placeholder="Enter username"
                    value={staffCreds.username}
                    onChange={e => setStaffCreds({ ...staffCreds, username: e.target.value })}
                    autoFocus
                    required
                  />
                </div>
              </div>
              <div className="form-group">
                <label htmlFor="password">Password</label>
                <div className="input-icon-wrap">
                  <span className="input-icon">🔑</span>
                  <input
                    type="password"
                    id="password"
                    className="form-control with-icon"
                    placeholder="Enter password"
                    value={staffCreds.password}
                    onChange={e => setStaffCreds({ ...staffCreds, password: e.target.value })}
                    required
                  />
                </div>
              </div>
              <button type="submit" className="btn-login" disabled={loading}>
                {loading ? <span className="btn-spinner" /> : '🔐 Sign In'}
              </button>
              <div className="login-hint">
                <strong>Demo:</strong> admin / admin123
              </div>
            </form>
          )}
        </div>
      </div>
    </div>
  );
};

export default Login;
