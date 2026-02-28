import React from 'react';
import { Navigate } from 'react-router-dom';
import { useAuth } from '../context/AuthContext';

/**
 * PrivateRoute – guards a route.
 * Props:
 *   allowedRoles  – array of roles that may access this route (optional)
 *                   e.g. ['admin','user']  or  ['patient']
 *                   If omitted, any authenticated user is allowed.
 */
const PrivateRoute = ({ children, allowedRoles }) => {
  const { isAuthenticated, loading, user } = useAuth();

  if (loading) {
    return (
      <div className="loading">
        <div className="spinner"></div>
      </div>
    );
  }

  if (!isAuthenticated) return <Navigate to="/login" replace />;

  // Role check
  if (allowedRoles && allowedRoles.length > 0) {
    if (!allowedRoles.includes(user?.role)) {
      // Patients get redirected to their portal; others to dashboard
      return <Navigate to={user?.role === 'patient' ? '/my-portal' : '/dashboard'} replace />;
    }
  }

  return children;
};

export default PrivateRoute;
