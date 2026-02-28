// Patient Controller
const { db } = require('../config/postgres');
const Patient = require('../models/Patient');

class PatientController {
  // Get all patients with optional filtering
  async getAllPatients(req, res) {
    try {
      const { 
        riskCategory, 
        minRiskScore, 
        maxRiskScore, 
        sex,
        minAge,
        maxAge,
        search,
        page = 1,
        limit = 50
      } = req.query;

      // Build SQL query with filters
      let conditions = [];
      let params = [];
      let paramCount = 0;

      if (riskCategory) {
        paramCount++;
        conditions.push(`risk_category = $${paramCount}`);
        params.push(riskCategory.toUpperCase());
      }

      if (minRiskScore) {
        paramCount++;
        conditions.push(`risk_score >= $${paramCount}`);
        params.push(parseFloat(minRiskScore));
      }

      if (maxRiskScore) {
        paramCount++;
        conditions.push(`risk_score <= $${paramCount}`);
        params.push(parseFloat(maxRiskScore));
      }

      if (sex) {
        paramCount++;
        conditions.push(`sex = $${paramCount}`);
        params.push(sex.toUpperCase());
      }

      if (minAge) {
        paramCount++;
        conditions.push(`age >= $${paramCount}`);
        params.push(parseInt(minAge));
      }

      if (maxAge) {
        paramCount++;
        conditions.push(`age <= $${paramCount}`);
        params.push(parseInt(maxAge));
      }

      if (search) {
        paramCount++;
        conditions.push(`(patient_id ILIKE $${paramCount} OR dl_classification ILIKE $${paramCount})`);
        params.push(`%${search}%`);
      }

      const whereClause = conditions.length > 0 ? `WHERE ${conditions.join(' AND ')}` : '';
      
      // Get total count
      const countQuery = `SELECT COUNT(*) FROM patients ${whereClause}`;
      const countResult = await db.query(countQuery, params);
      const total = parseInt(countResult.rows[0].count);

      // Get paginated results
      const offset = (page - 1) * limit;
      paramCount++;
      const limitParam = `$${paramCount}`;
      params.push(limit);
      paramCount++;
      const offsetParam = `$${paramCount}`;
      params.push(offset);

      const query = `
        SELECT * FROM patients 
        ${whereClause}
        ORDER BY risk_score DESC
        LIMIT ${limitParam} OFFSET ${offsetParam}
      `;
      
      const result = await db.query(query, params);

      res.json({
        success: true,
        data: result.rows,
        pagination: {
          total,
          page: parseInt(page),
          limit: parseInt(limit),
          totalPages: Math.ceil(total / limit)
        }
      });
    } catch (error) {
      console.error('Get all patients error:', error);
      res.status(500).json({
        success: false,
        message: 'Failed to retrieve patients',
        error: error.message
      });
    }
  }

  // Get patient by ID
  async getPatientById(req, res) {
    try {
      const { id } = req.params;
      const patientData = await db.getPatientById(id);

      if (!patientData) {
        return res.status(404).json({
          success: false,
          message: 'Patient not found'
        });
      }

      res.json({
        success: true,
        data: patientData
      });

      res.json({
        success: true,
        data: patient.toJSON()
      });
    } catch (error) {
      console.error('Get patient by ID error:', error);
      res.status(500).json({
        success: false,
        message: 'Failed to retrieve patient'
      });
    }
  }

  // Get abnormal patients
  async getAbnormalPatients(req, res) {
    try {
      const abnormalPatients = database.getAbnormalPatients();

      res.json({
        success: true,
        data: abnormalPatients.map(p => new Patient(p).toJSON()),
        count: abnormalPatients.length
      });
    } catch (error) {
      console.error('Get abnormal patients error:', error);
      res.status(500).json({
        success: false,
        message: 'Failed to retrieve abnormal patients'
      });
    }
  }

  // Get patients by date range
  async getPatientsByDateRange(req, res) {
    try {
      const { startDate, endDate } = req.query;

      if (!startDate || !endDate) {
        return res.status(400).json({
          success: false,
          message: 'Start date and end date are required'
        });
      }

      const patients = database.getPatientsByDateRange(startDate, endDate);

      res.json({
        success: true,
        data: patients.map(p => new Patient(p).toJSON()),
        count: patients.length,
        dateRange: { startDate, endDate }
      });
    } catch (error) {
      console.error('Get patients by date range error:', error);
      res.status(500).json({
        success: false,
        message: 'Failed to retrieve patients by date range'
      });
    }
  }

  // Create new patient record
  async createPatient(req, res) {
    try {
      const patientData = req.body;

      // Validate patient data
      const validation = Patient.validatePatientData(patientData);
      if (!validation.valid) {
        return res.status(400).json({
          success: false,
          errors: validation.errors
        });
      }

      // Check if patient already exists
      const existingPatient = database.getPatientById(patientData.patient_id);
      if (existingPatient) {
        return res.status(409).json({
          success: false,
          message: 'Patient with this ID already exists'
        });
      }

      // Create patient
      const patient = new Patient(patientData);
      database.createPatient(patient);

      res.status(201).json({
        success: true,
        message: 'Patient record created successfully',
        data: patient.toJSON()
      });
    } catch (error) {
      console.error('Create patient error:', error);
      res.status(500).json({
        success: false,
        message: 'Failed to create patient record'
      });
    }
  }

  // Update patient record
  async updatePatient(req, res) {
    try {
      const { id } = req.params;
      const updates = req.body;

      const existingPatient = database.getPatientById(id);
      if (!existingPatient) {
        return res.status(404).json({
          success: false,
          message: 'Patient not found'
        });
      }

      // Merge updates with existing data
      const updatedData = { ...existingPatient, ...updates, patient_id: id };

      // Validate updated data
      const validation = Patient.validatePatientData(updatedData);
      if (!validation.valid) {
        return res.status(400).json({
          success: false,
          errors: validation.errors
        });
      }

      // Update patient
      const patient = new Patient(updatedData);
      database.updatePatient(id, patient);

      res.json({
        success: true,
        message: 'Patient record updated successfully',
        data: patient.toJSON()
      });
    } catch (error) {
      console.error('Update patient error:', error);
      res.status(500).json({
        success: false,
        message: 'Failed to update patient record'
      });
    }
  }

  // Delete patient record
  async deletePatient(req, res) {
    try {
      const { id } = req.params;

      const existingPatient = database.getPatientById(id);
      if (!existingPatient) {
        return res.status(404).json({
          success: false,
          message: 'Patient not found'
        });
      }

      database.deletePatient(id);

      res.json({
        success: true,
        message: 'Patient record deleted successfully'
      });
    } catch (error) {
      console.error('Delete patient error:', error);
      res.status(500).json({
        success: false,
        message: 'Failed to delete patient record'
      });
    }
  }

  // Get patient summary statistics
  async getPatientSummary(req, res) {
    try {
      const { id } = req.params;
      const patientData = database.getPatientById(id);

      if (!patientData) {
        return res.status(404).json({
          success: false,
          message: 'Patient not found'
        });
      }

      const patient = new Patient(patientData);

      res.json({
        success: true,
        data: patient.getSummary()
      });
    } catch (error) {
      console.error('Get patient summary error:', error);
      res.status(500).json({
        success: false,
        message: 'Failed to retrieve patient summary'
      });
    }
  }

  // Search patients
  async searchPatients(req, res) {
    try {
      const { q } = req.query;

      if (!q) {
        return res.status(400).json({
          success: false,
          message: 'Search query is required'
        });
      }

      const allPatients = database.getAllPatients();
      const searchLower = q.toLowerCase();

      const results = allPatients.filter(p => 
        p.patient_id.toLowerCase().includes(searchLower) ||
        p.classification.toLowerCase().includes(searchLower) ||
        p.sex.toLowerCase().includes(searchLower)
      );

      res.json({
        success: true,
        data: results.map(p => new Patient(p).toJSON()),
        count: results.length,
        query: q
      });
    } catch (error) {
      console.error('Search patients error:', error);
      res.status(500).json({
        success: false,
        message: 'Failed to search patients'
      });
    }
  }
}

module.exports = new PatientController();
