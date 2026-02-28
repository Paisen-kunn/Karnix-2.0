// Database Configuration
// Using in-memory storage for this implementation
// Can be replaced with MongoDB, PostgreSQL, etc.

class Database {
  constructor() {
    this.users = new Map();
    this.patients = new Map();
    this.sessions = new Map();
    
    // Initialize with admin user
    this.initializeAdmin();
  }

  initializeAdmin() {
    const bcrypt = require('bcryptjs');
    const adminPassword = bcrypt.hashSync('admin123', 10);
    
    this.users.set('admin', {
      id: 'admin',
      username: 'admin',
      email: 'admin@spine-analytics.com',
      password: adminPassword,
      role: 'admin',
      isActive: true,
      createdAt: new Date().toISOString()
    });
  }

  // User Operations
  createUser(user) {
    if (this.users.has(user.username)) {
      throw new Error('User already exists');
    }
    this.users.set(user.username, user);
    return user;
  }

  findUserByUsername(username) {
    return this.users.get(username);
  }

  findUserById(id) {
    for (let user of this.users.values()) {
      if (user.id === id) return user;
    }
    return null;
  }

  getAllUsers() {
    return Array.from(this.users.values());
  }

  updateUser(username, updates) {
    const user = this.users.get(username);
    if (!user) return null;
    
    const updatedUser = { ...user, ...updates };
    this.users.set(username, updatedUser);
    return updatedUser;
  }

  deleteUser(username) {
    return this.users.delete(username);
  }

  // Patient Operations
  createPatient(patient) {
    this.patients.set(patient.patient_id, patient);
    return patient;
  }

  findPatientById(patientId) {
    return this.patients.get(patientId);
  }

  getPatientById(patientId) {
    return this.patients.get(patientId);
  }

  getAllPatients() {
    return Array.from(this.patients.values());
  }

  updatePatient(patientId, updates) {
    const patient = this.patients.get(patientId);
    if (!patient) return null;
    
    const updatedPatient = { ...patient, ...updates };
    this.patients.set(patientId, updatedPatient);
    return updatedPatient;
  }

  deletePatient(patientId) {
    return this.patients.delete(patientId);
  }

  bulkInsertPatients(patients) {
    patients.forEach(patient => {
      this.patients.set(patient.patient_id, patient);
    });
    return patients.length;
  }

  // Session Operations
  createSession(sessionId, userId) {
    this.sessions.set(sessionId, {
      userId,
      createdAt: new Date().toISOString(),
      expiresAt: new Date(Date.now() + 24 * 60 * 60 * 1000).toISOString()
    });
  }

  getSession(sessionId) {
    return this.sessions.get(sessionId);
  }

  deleteSession(sessionId) {
    return this.sessions.delete(sessionId);
  }

  // Analytics Queries
  getPatientsByDateRange(startDate, endDate) {
    return Array.from(this.patients.values()).filter(p => {
      const patientDate = new Date(p.date);
      return patientDate >= new Date(startDate) && patientDate <= new Date(endDate);
    });
  }

  getAbnormalPatients() {
    return Array.from(this.patients.values()).filter(p => 
      p.classification === 'Abnormal'
    );
  }

  getRiskDistribution() {
    const patients = Array.from(this.patients.values());
    return {
      high: patients.filter(p => p.risk_score > 70).length,
      moderate: patients.filter(p => p.risk_score >= 30 && p.risk_score <= 70).length,
      low: patients.filter(p => p.risk_score < 30).length
    };
  }
}

module.exports = new Database();
