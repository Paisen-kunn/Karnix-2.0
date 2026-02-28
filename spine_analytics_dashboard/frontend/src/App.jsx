import React from 'react';
import { BrowserRouter as Router, Routes, Route, Navigate } from 'react-router-dom';
import { AuthProvider } from './context/AuthContext';
import Navbar from './components/Navbar';
import PrivateRoute from './components/PrivateRoute';
import Login from './pages/Login';
import Dashboard from './pages/Dashboard';
import Patients from './pages/Patients';
import Analytics from './pages/Analytics';
import PatientPortal from './pages/PatientPortal';
import './App.css';

function App() {
  return (
    <AuthProvider>
      <Router>
        <div className="App">
          <Navbar />
          <div className="container">
            <Routes>
              {/* Public */}
              <Route path="/login" element={<Login />} />

              {/* Patient-only */}
              <Route
                path="/my-portal"
                element={
                  <PrivateRoute allowedRoles={['patient']}>
                    <PatientPortal />
                  </PrivateRoute>
                }
              />

              {/* Staff / Admin only */}
              <Route
                path="/dashboard"
                element={
                  <PrivateRoute allowedRoles={['admin', 'user']}>
                    <Dashboard />
                  </PrivateRoute>
                }
              />
              <Route
                path="/patients"
                element={
                  <PrivateRoute allowedRoles={['admin', 'user']}>
                    <Patients />
                  </PrivateRoute>
                }
              />
              <Route
                path="/analytics"
                element={
                  <PrivateRoute allowedRoles={['admin', 'user']}>
                    <Analytics />
                  </PrivateRoute>
                }
              />

              {/* Default redirect — after login, PrivateRoute handles role routing */}
              <Route path="/" element={<Navigate to="/login" replace />} />
              <Route path="*" element={<Navigate to="/login" replace />} />
            </Routes>
          </div>
        </div>
      </Router>
    </AuthProvider>
  );
}

export default App;
