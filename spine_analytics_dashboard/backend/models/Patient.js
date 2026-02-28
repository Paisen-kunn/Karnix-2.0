// Patient Model
const { v4: uuidv4 } = require('uuid');

class Patient {
  constructor(data) {
    this.id = data.id || uuidv4();
    this.patient_id = data.patient_id;
    this.age = data.age || 0;
    this.sex = data.sex || 'N/A';
    this.date = data.date;
    this.risk_score = parseFloat(data.risk_score) || 0;
    this.abnormal_prob = parseFloat(data.abnormal_prob) || 0;
    this.classification = data.classification;
    this.image_path = data.image_path;
    this.createdAt = data.createdAt || new Date().toISOString();
  }

  // Calculate risk category
  getRiskCategory() {
    if (this.risk_score >= 70) return 'HIGH';
    if (this.risk_score >= 30) return 'MODERATE';
    return 'LOW';
  }

  // Check if abnormal
  isAbnormal() {
    return this.classification === 'Abnormal';
  }

  // Validate patient data
  static validate(data) {
    const errors = [];

    if (!data.patient_id) {
      errors.push('Patient ID is required');
    }

    if (!data.date) {
      errors.push('Date is required');
    }

    if (data.risk_score === undefined || isNaN(data.risk_score)) {
      errors.push('Valid risk score is required');
    }

    if (!data.classification) {
      errors.push('Classification is required');
    }

    return {
      valid: errors.length === 0,
      errors
    };
  }

  // Get summary
  getSummary() {
    return {
      patient_id: this.patient_id,
      risk_category: this.getRiskCategory(),
      classification: this.classification,
      risk_score: this.risk_score,
      abnormal_prob: this.abnormal_prob,
      date: this.date
    };
  }

  // Convert to JSON
  toJSON() {
    return {
      id: this.id,
      patient_id: this.patient_id,
      age: this.age,
      sex: this.sex,
      date: this.date,
      risk_score: this.risk_score,
      abnormal_prob: this.abnormal_prob,
      classification: this.classification,
      risk_category: this.getRiskCategory(),
      image_path: this.image_path,
      createdAt: this.createdAt
    };
  }
}

module.exports = Patient;
