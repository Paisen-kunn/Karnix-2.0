const { db } = require('../config/postgres');

class VitalsController {
  // Get recent vitals for a patient
  async getPatientVitals(req, res) {
    try {
      const { patientId } = req.params;
      const limit = parseInt(req.query.limit) || 100;
      
      const vitals = await db.getRecentVitals(patientId, limit);
      
      res.json({
        success: true,
        data: vitals,
        count: vitals.length
      });
    } catch (error) {
      res.status(500).json({
        success: false,
        message: 'Failed to fetch vitals',
        error: error.message
      });
    }
  }

  // Get latest vitals for all patients
  async getLatestVitals(req, res) {
    try {
      const result = await db.query(`
        SELECT DISTINCT ON (patient_id) *
        FROM health_vitals
        ORDER BY patient_id, timestamp DESC
      `);
      
      res.json({
        success: true,
        data: result.rows
      });
    } catch (error) {
      res.status(500).json({
        success: false,
        message: 'Failed to fetch latest vitals',
        error: error.message
      });
    }
  }

  // Get real-time metrics
  async getRealtimeMetrics(req, res) {
    try {
      const limit = parseInt(req.query.limit) || 50;
      const metricType = req.query.type;
      
      let metrics;
      if (metricType) {
        metrics = await db.getMetricsByType(metricType, limit);
      } else {
        metrics = await db.getRecentMetrics(limit);
      }
      
      res.json({
        success: true,
        data: metrics
      });
    } catch (error) {
      res.status(500).json({
        success: false,
        message: 'Failed to fetch real-time metrics',
        error: error.message
      });
    }
  }

  // Get vitals statistics
  async getVitalsStats(req, res) {
    try {
      const { patientId } = req.params;
      
      const result = await db.query(`
        SELECT 
          AVG(heart_rate) as avg_heart_rate,
          MIN(heart_rate) as min_heart_rate,
          MAX(heart_rate) as max_heart_rate,
          AVG(spo2) as avg_spo2,
          MIN(spo2) as min_spo2,
          MAX(spo2) as max_spo2,
          AVG(temperature) as avg_temperature,
          AVG(sleep_hours) as avg_sleep_hours,
          COUNT(*) as total_readings
        FROM health_vitals
        WHERE patient_id = $1
      `, [patientId]);
      
      res.json({
        success: true,
        data: result.rows[0]
      });
    } catch (error) {
      res.status(500).json({
        success: false,
        message: 'Failed to fetch vitals statistics',
        error: error.message
      });
    }
  }

  // Get heart rate trends
  async getHeartRateTrends(req, res) {
    try {
      const hours = parseInt(req.query.hours) || 24;
      
      const result = await db.query(`
        SELECT 
          DATE_TRUNC('minute', timestamp) as time,
          AVG(heart_rate) as avg_heart_rate,
          patient_id
        FROM health_vitals
        WHERE timestamp >= NOW() - INTERVAL '${hours} hours'
        GROUP BY time, patient_id
        ORDER BY time DESC
        LIMIT 100
      `);
      
      res.json({
        success: true,
        data: result.rows
      });
    } catch (error) {
      res.status(500).json({
        success: false,
        message: 'Failed to fetch heart rate trends',
        error: error.message
      });
    }
  }

  // Get SpO2 trends
  async getSpO2Trends(req, res) {
    try {
      const hours = parseInt(req.query.hours) || 24;
      
      const result = await db.query(`
        SELECT 
          DATE_TRUNC('minute', timestamp) as time,
          AVG(spo2) as avg_spo2,
          patient_id
        FROM health_vitals
        WHERE timestamp >= NOW() - INTERVAL '${hours} hours'
        GROUP BY time, patient_id
        ORDER BY time DESC
        LIMIT 100
      `);
      
      res.json({
        success: true,
        data: result.rows
      });
    } catch (error) {
      res.status(500).json({
        success: false,
        message: 'Failed to fetch SpO2 trends',
        error: error.message
      });
    }
  }
}

module.exports = new VitalsController();
