import React from 'react';
import { Link, useLocation } from 'react-router-dom';
import { useAuth } from '../context/AuthContext';

const Navbar = () => {
  const { user, logout, isPatient, isAdmin } = useAuth();
  const location = useLocation();

  if (!user) return null;

  const active = (path) =>
    location.pathname === path ? ' active' : '';

  return (
    <nav className="navbar">
      <Link
        to={isPatient ? '/my-portal' : '/dashboard'}
        className="navbar-brand"
      >
        🦴 Spine Analytics
      </Link>

      <ul className="navbar-nav">
        {/* ── Patient view: only My Records ── */}
        {isPatient && (
          <li>
            <Link to="/my-portal" className={`nav-link${active('/my-portal')}`}>
              🩻 My Records
            </Link>
          </li>
        )}

        {/* ── Staff / Admin view ── */}
        {!isPatient && (
          <>
            <li>
              <Link to="/dashboard" className={`nav-link${active('/dashboard')}`}>
                📊 Dashboard
              </Link>
            </li>
            <li>
              <Link to="/patients" className={`nav-link${active('/patients')}`}>
                👥 Patients
              </Link>
            </li>
            <li>
              <Link to="/analytics" className={`nav-link${active('/analytics')}`}>
                📈 Analytics
              </Link>
            </li>
          </>
        )}
      </ul>

      <div className="user-info">
        <div className="user-badge">
          <div className="user-avatar">
            {isPatient ? '🧑‍⚕️' : isAdmin ? '👨‍💼' : '👤'}
          </div>
          <div className="user-details">
            <span className="user-name">{user.username}</span>
            <span
              className="role-badge"
              style={{
                background: isPatient
                  ? '#0ea5e9'
                  : isAdmin
                  ? '#7c3aed'
                  : '#059669'
              }}
            >
              {isPatient ? 'PATIENT' : isAdmin ? 'ADMIN' : 'STAFF'}
            </span>
          </div>
        </div>
        <button onClick={logout} className="btn-logout">
          Sign out
        </button>
      </div>
    </nav>
  );
};

export default Navbar;
