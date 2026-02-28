// User Model
const bcrypt = require('bcryptjs');
const { v4: uuidv4 } = require('uuid');
const jwtConfig = require('../config/jwt');

class User {
  constructor(data) {
    this.id = data.id || uuidv4();
    this.username = data.username;
    this.email = data.email;
    this.password = data.password; // Will be hashed
    this.role = data.role || 'user';
    this.firstName = data.firstName || '';
    this.lastName = data.lastName || '';
    this.createdAt = data.createdAt || new Date().toISOString();
    this.updatedAt = data.updatedAt || new Date().toISOString();
    this.lastLogin = data.lastLogin || null;
    this.isActive = data.isActive !== undefined ? data.isActive : true;
  }

  // Hash password
  static async hashPassword(password) {
    return await bcrypt.hash(password, jwtConfig.bcryptRounds);
  }

  // Compare password
  static async comparePassword(plainPassword, hashedPassword) {
    return await bcrypt.compare(plainPassword, hashedPassword);
  }

  // Validate user data
  static validate(data) {
    const errors = [];

    if (!data.username || data.username.length < 3) {
      errors.push('Username must be at least 3 characters long');
    }

    if (!data.email || !/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(data.email)) {
      errors.push('Valid email is required');
    }

    if (!data.password || data.password.length < 6) {
      errors.push('Password must be at least 6 characters long');
    }

    return {
      valid: errors.length === 0,
      errors
    };
  }

  // Remove sensitive data
  toJSON() {
    const { password, ...userWithoutPassword } = this;
    return userWithoutPassword;
  }

  // Get safe user object (without password)
  getSafeUser() {
    return {
      id: this.id,
      username: this.username,
      email: this.email,
      role: this.role,
      firstName: this.firstName,
      lastName: this.lastName,
      createdAt: this.createdAt,
      lastLogin: this.lastLogin
    };
  }
}

module.exports = User;
