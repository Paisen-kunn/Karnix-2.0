// Analytics Controller
const { db } = require('../config/postgres');

class AnalyticsController {
  // Get comprehensive dashboard statistics
  async getDashboardStats(req, res) {
    try {
      const stats = await db.getPatientStats();
      
      // Get recent scans (last 30 days)
      const recentScansResult = await db.query(`
        SELECT COUNT(*) as count
        FROM patients
        WHERE scan_date >= NOW() - INTERVAL '30 days'
      `);

      // Get gender distribution
      const genderDist = await db.getGenderDistribution();
      let maleCount = 0;
      let femaleCount = 0;
      genderDist.forEach(item => {
        if (item.gender === 'M') maleCount = parseInt(item.count);
        else if (item.gender === 'F') femaleCount = parseInt(item.count);
      });

      const totalPatients = parseInt(stats.total_patients) || 0;
      const malePercentage = totalPatients > 0 ? ((maleCount / totalPatients) * 100).toFixed(2) + '%' : '0%';
      const femalePercentage = totalPatients > 0 ? ((femaleCount / totalPatients) * 100).toFixed(2) + '%' : '0%';

      res.json({
        success: true,
        data: {
          overview: {
            totalPatients,
            abnormalCases: parseInt(stats.abnormal_cases) || 0,
            abnormalRate: stats.abnormal_cases > 0 
              ? ((stats.abnormal_cases / stats.total_patients) * 100).toFixed(2) + '%'
              : '0%',
            recentScans: parseInt(recentScansResult.rows[0].count) || 0
          },
          demographics: {
            male: maleCount,
            female: femaleCount,
            malePercentage,
            femalePercentage
          },
          ageStatistics: {
            average: parseFloat(stats.avg_age || 0).toFixed(1),
            minimum: 0,
            maximum: 100
          },
          riskScoreStatistics: {
            average: parseFloat(stats.avg_risk_score || 0).toFixed(2),
          }
        }
      });
    } catch (error) {
      console.error('Dashboard stats error:', error);
      res.status(500).json({
        success: false,
        message: 'Failed to retrieve dashboard statistics',
        error: error.message
      });
    }
  }

  // Get risk distribution
  async getRiskDistribution(req, res) {
    try {
      const distribution = await db.getRiskDistribution();
      
      const result = {
        high: 0,
        moderate: 0,
        low: 0
      };

      distribution.forEach(item => {
        const category = item.risk_category?.toLowerCase();
        const count = parseInt(item.count);
        if (category === 'high') result.high = count;
        else if (category === 'moderate') result.moderate = count;
        else if (category === 'low') result.low = count;
      });

      res.json({
        success: true,
        data: result
      });
    } catch (error) {
      console.error('Risk distribution error:', error);
      res.status(500).json({
        success: false,
        message: 'Failed to retrieve risk distribution',
        error: error.message
      });
    }
  }

  // Get gender distribution
  async getGenderDistribution(req, res) {
    try {
      const distribution = await db.getGenderDistribution();
      
      const result = {
        male: 0,
        female: 0
      };

      distribution.forEach(item => {
        const gender = item.gender?.toUpperCase();
        const count = parseInt(item.count);
        if (gender === 'M') result.male = count;
        else if (gender === 'F') result.female = count;
      });

      res.json({
        success: true,
        data: result
      });
    } catch (error) {
      console.error('Gender distribution error:', error);
      res.status(500).json({
        success: false,
        message: 'Failed to retrieve gender distribution',
        error: error.message
      });
    }
  }

  // Get classification breakdown
  async getClassificationBreakdown(req, res) {
    try {
      const result = await db.query(`
        SELECT 
          dl_classification,
          COUNT(*) as count
        FROM patients
        WHERE dl_classification IS NOT NULL
        GROUP BY dl_classification
      `);

      const breakdown = {
        normal: 0,
        abnormal: 0
      };

      result.rows.forEach(item => {
        const classification = item.dl_classification?.toUpperCase();
        const count = parseInt(item.count);
        if (classification === 'NORMAL') breakdown.normal = count;
        else if (classification === 'ABNORMAL') breakdown.abnormal = count;
      });

      res.json({
        success: true,
        data: breakdown
      });
    } catch (error) {
      console.error('Classification breakdown error:', error);
      res.status(500).json({
        success: false,
        message: 'Failed to retrieve classification breakdown',
        error: error.message
      });
    }
  }

  // Get trends over time
  async getTrends(req, res) {
    try {
      const { period = 30 } = req.query;
      
      const result = await db.query(`
        SELECT 
          DATE(scan_date) as date,
          COUNT(*) as total_scans,
          COUNT(CASE WHEN dl_classification = 'ABNORMAL' THEN 1 END) as abnormal_cases
        FROM patients
        WHERE scan_date >= NOW() - INTERVAL '${parseInt(period)} days'
        GROUP BY DATE(scan_date)
        ORDER BY date ASC
      `);

      res.json({
        success: true,
        data: result.rows
      });
    } catch (error) {
      console.error('Trends error:', error);
      res.status(500).json({
        success: false,
        message: 'Failed to retrieve trends',
        error: error.message
      });
    }
  }

  // Get abnormality rates
  async getAbnormalityRates(req, res) {
    try {
      // Overall rates
      const overallResult = await db.query(`
        SELECT 
          COUNT(*) as total,
          COUNT(CASE WHEN dl_classification = 'ABNORMAL' THEN 1 END) as abnormal
        FROM patients
      `);

      const overall = overallResult.rows[0];
      const overallRate = overall.total > 0 
        ? ((overall.abnormal / overall.total) * 100).toFixed(2) + '%'
        : '0%';

      // By gender
      const genderResult = await db.query(`
        SELECT 
          sex,
          COUNT(*) as total,
          COUNT(CASE WHEN dl_classification = 'ABNORMAL' THEN 1 END) as abnormal
        FROM patients
        WHERE sex IS NOT NULL
        GROUP BY sex
      `);

      const byGender = {
        male: { total: 0, abnormal: 0, rate: '0%' },
        female: { total: 0, abnormal: 0, rate: '0%' }
      };

      genderResult.rows.forEach(row => {
        const total = parseInt(row.total);
        const abnormal = parseInt(row.abnormal);
        const rate = total > 0 ? ((abnormal / total) * 100).toFixed(2) + '%' : '0%';
        
        if (row.sex === 'M') {
          byGender.male = { total, abnormal, rate };
        } else if (row.sex === 'F') {
          byGender.female = { total, abnormal, rate };
        }
      });

      // By age group
      const ageGroupResult = await db.query(`
        SELECT 
          CASE 
            WHEN age < 21 THEN '0-20'
            WHEN age BETWEEN 21 AND 40 THEN '21-40'
            WHEN age BETWEEN 41 AND 60 THEN '41-60'
            WHEN age BETWEEN 61 AND 80 THEN '61-80'
            ELSE '81+'
          END as age_group,
          COUNT(*) as total,
          COUNT(CASE WHEN dl_classification = 'ABNORMAL' THEN 1 END) as abnormal
        FROM patients
        WHERE age IS NOT NULL
        GROUP BY age_group
        ORDER BY age_group
      `);

      const byAgeGroup = {};
      ageGroupResult.rows.forEach(row => {
        const total = parseInt(row.total);
        const abnormal = parseInt(row.abnormal);
        const rate = total > 0 ? ((abnormal / total) * 100).toFixed(2) + '%' : '0%';
        byAgeGroup[row.age_group] = { total, abnormal, rate };
      });

      res.json({
        success: true,
        data: {
          overall: {
            total: parseInt(overall.total),
            abnormal: parseInt(overall.abnormal),
            rate: overallRate
          },
          byGender,
          byAgeGroup
        }
      });
    } catch (error) {
      console.error('Abnormality rates error:', error);
      res.status(500).json({
        success: false,
        message: 'Failed to retrieve abnormality rates',
        error: error.message
      });
    }
  }

  // Get high-risk patients
  async getHighRiskPatients(req, res) {
    try {
      const { limit = 20 } = req.query;
      
      const result = await db.query(`
        SELECT * 
        FROM patients
        WHERE risk_category = 'HIGH'
        ORDER BY risk_score DESC
        LIMIT $1
      `, [parseInt(limit)]);

      res.json({
        success: true,
        data: result.rows
      });
    } catch (error) {
      console.error('High-risk patients error:', error);
      res.status(500).json({
        success: false,
        message: 'Failed to retrieve high-risk patients',
        error: error.message
      });
    }
  }

  // Export data
  async exportData(req, res) {
    try {
      const { format = 'json' } = req.query;
      
      const patients = await db.getAllPatients(1000);

      if (format === 'csv') {
        // Convert to CSV
        const headers = Object.keys(patients[0] || {});
        const csv = [
          headers.join(','),
          ...patients.map(p => headers.map(h => JSON.stringify(p[h] || '')).join(','))
        ].join('\n');

        res.setHeader('Content-Type', 'text/csv');
        res.setHeader('Content-Disposition', 'attachment; filename=spine_data.csv');
        res.send(csv);
      } else {
        res.json({
          success: true,
          data: patients,
          count: patients.length,
          exportedAt: new Date().toISOString()
        });
      }
    } catch (error) {
      console.error('Export data error:', error);
      res.status(500).json({
        success: false,
        message: 'Failed to export data',
        error: error.message
      });
    }
  }
}

module.exports = new AnalyticsController();
