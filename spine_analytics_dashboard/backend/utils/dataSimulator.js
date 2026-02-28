const { db } = require('../config/postgres');

class DataSimulator {
  constructor() {
    this.isRunning = false;
    this.interval = null;
    this.patientIds = [];
  }

  // Generate random value within range
  random(min, max, decimals = 0) {
    const value = Math.random() * (max - min) + min;
    return decimals > 0 ? parseFloat(value.toFixed(decimals)) : Math.floor(value);
  }

  // Generate realistic health vitals
  generateHealthVitals(patientId) {
    return {
      patient_id: patientId,
      heart_rate: this.random(60, 100),
      spo2: this.random(95, 100, 2),
      blood_pressure_systolic: this.random(110, 140),
      blood_pressure_diastolic: this.random(70, 90),
      temperature: this.random(36.5, 37.5, 1),
      respiratory_rate: this.random(12, 20),
      steps: this.random(0, 10000),
      calories_burned: this.random(1500, 3000),
      sleep_hours: this.random(4, 10, 1),
      sleep_quality: ['Excellent', 'Good', 'Fair', 'Poor'][this.random(0, 3)],
      stress_level: this.random(1, 10),
      activity_level: ['Sedentary', 'Light', 'Moderate', 'Active', 'Very Active'][this.random(0, 4)]
    };
  }

  // Generate comprehensive spine patient data
  generateSpinePatient(patientId) {
    const age = this.random(20, 80);
    const sex = Math.random() > 0.5 ? 'M' : 'F';
    
    // Deep Learning probabilities
    const abnormalProb = this.random(0, 100, 2);
    const normalProb = (100 - abnormalProb).toFixed(2);
    const dlClassification = abnormalProb > 50 ? 'ABNORMAL' : 'NORMAL';
    
    // UCI Vertebral Column measurements
    const pelvicIncidence = this.random(40, 80, 2);
    const pelvicTilt = this.random(10, 30, 2);
    const lumbarLordosisAngle = this.random(40, 70, 2);
    const sacralSlope = this.random(30, 50, 2);
    const pelvicRadius = this.random(100, 150, 2);
    const gradeOfSpondylolisthesis = this.random(0, 5, 2);
    
    // Calculate risk score based on measurements
    let riskScore = this.random(0, 100, 2);
    let riskCategory = 'LOW';
    if (riskScore > 70) riskCategory = 'HIGH';
    else if (riskScore > 40) riskCategory = 'MODERATE';
    
    const pathologies = ['Normal', 'Disk Hernia', 'Spondylolisthesis'];
    const pathology = pathologies[this.random(0, pathologies.length - 1)];
    
    // Clinical scoring
    const cobbAngle = this.random(5, 40, 2);
    let scoliosisSeverity = 'Mild';
    if (cobbAngle > 25) scoliosisSeverity = 'Severe';
    else if (cobbAngle > 15) scoliosisSeverity = 'Moderate';
    
    const compressionFractureGrade = this.random(0, 3);
    const boneDensityTscore = this.random(-3.0, 2.0, 2);
    let boneDensityClassification = 'Normal';
    if (boneDensityTscore < -2.5) boneDensityClassification = 'Osteoporosis';
    else if (boneDensityTscore < -1.0) boneDensityClassification = 'Osteopenia';
    
    const fractureRisk10yr = this.random(5, 40, 2);
    const discDegenerationGrade = this.random(1, 5);
    const instabilityScore = this.random(0, 13);
    
    return {
      patient_id: patientId,
      age,
      sex,
      normal_prob: normalProb,
      abnormal_prob: abnormalProb,
      dl_classification: dlClassification,
      pelvic_incidence: pelvicIncidence,
      pelvic_tilt: pelvicTilt,
      lumbar_lordosis_angle: lumbarLordosisAngle,
      sacral_slope: sacralSlope,
      pelvic_radius: pelvicRadius,
      grade_of_spondylolisthesis: gradeOfSpondylolisthesis,
      pathology,
      risk_score: riskScore,
      risk_category: riskCategory,
      cobb_angle: cobbAngle,
      scoliosis_severity: scoliosisSeverity,
      compression_fracture_grade: compressionFractureGrade,
      bone_density_tscore: boneDensityTscore,
      bone_density_classification: boneDensityClassification,
      fracture_risk_10yr: fractureRisk10yr,
      disc_degeneration_grade: discDegenerationGrade,
      instability_score: instabilityScore,
      image_path: `/data/images/spine_${patientId}.jpg`,
      processing_time_ms: this.random(50, 150, 2)
    };
  }

  // Start simulation
  async start(intervalMs = 1000) {
    if (this.isRunning) {
      console.log('⚠️  Simulator already running');
      return;
    }

    console.log('🚀 Starting data simulator...');
    this.isRunning = true;

    // Generate initial patient if needed
    try {
      const patients = await db.getAllPatients(1);
      if (patients.length === 0) {
        // Create initial test patients
        for (let i = 1; i <= 5; i++) {
          const patientId = `SIM_${Date.now()}_${i}`;
          this.patientIds.push(patientId);
          await db.createPatient(this.generateSpinePatient(patientId));
        }
        console.log(`✅ Created ${this.patientIds.length} initial patients`);
      } else {
        // Use existing patient IDs
        this.patientIds = patients.map(p => p.patient_id);
        console.log(`✅ Using ${this.patientIds.length} existing patients`);
      }
    } catch (error) {
      console.error('Error initializing patients:', error);
    }

    // Start interval
    this.interval = setInterval(async () => {
      try {
        // Generate health vitals for random patient
        if (this.patientIds.length > 0) {
          const randomPatientId = this.patientIds[this.random(0, this.patientIds.length - 1)];
          const vitals = this.generateHealthVitals(randomPatientId);
          await db.createHealthVital(vitals);

          // Create real-time metrics
          await db.createMetric('heart_rate', vitals.heart_rate, 'bpm', randomPatientId);
          await db.createMetric('spo2', vitals.spo2, '%', randomPatientId);
          await db.createMetric('temperature', vitals.temperature, '°C', randomPatientId);

          const timestamp = new Date().toISOString();
          console.log(`📊 [${timestamp}] Generated vitals for ${randomPatientId}: HR=${vitals.heart_rate} SpO2=${vitals.spo2}%`);
        }

        // Randomly generate new patient (10% chance)
        if (Math.random() < 0.1) {
          const patientId = `SIM_${Date.now()}`;
          this.patientIds.push(patientId);
          await db.createPatient(this.generateSpinePatient(patientId));
          console.log(`👤 Created new patient: ${patientId}`);
        }
      } catch (error) {
        console.error('❌ Simulation error:', error.message);
      }
    }, intervalMs);

    console.log(`✅ Simulator running (interval: ${intervalMs}ms)`);
  }

  // Stop simulation
  stop() {
    if (!this.isRunning) {
      console.log('⚠️  Simulator not running');
      return;
    }

    clearInterval(this.interval);
    this.isRunning = false;
    console.log('🛑 Simulator stopped');
  }

  // Get status
  getStatus() {
    return {
      isRunning: this.isRunning,
      patientCount: this.patientIds.length,
      patients: this.patientIds
    };
  }
}

// Singleton instance
const simulator = new DataSimulator();

module.exports = simulator;
