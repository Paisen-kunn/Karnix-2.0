import React, { useState, useEffect } from 'react';
import { analyticsAPI } from '../services/api';

const Analytics = () => {
  const [abnormalityRates, setAbnormalityRates] = useState(null);
  const [highRiskPatients, setHighRiskPatients] = useState([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    loadAnalytics();
  }, []);

  const loadAnalytics = async () => {
    try {
      const [ratesRes, highRiskRes] = await Promise.all([
        analyticsAPI.getAbnormalityRates(),
        analyticsAPI.getHighRiskPatients()
      ]);

      setAbnormalityRates(ratesRes.data.data);
      setHighRiskPatients(highRiskRes.data.data);
    } catch (error) {
      console.error('Failed to load analytics:', error);
    } finally {
      setLoading(false);
    }
  };

  const handleExport = async (format) => {
    try {
      const response = await analyticsAPI.exportData(format);
      
      if (format === 'csv') {
        const blob = new Blob([response.data], { type: 'text/csv' });
        const url = window.URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = 'spine_data_export.csv';
        a.click();
      } else {
        const dataStr = JSON.stringify(response.data, null, 2);
        const blob = new Blob([dataStr], { type: 'application/json' });
        const url = window.URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = 'spine_data_export.json';
        a.click();
      }
    } catch (error) {
      console.error('Export failed:', error);
    }
  };

  if (loading) {
    return (
      <div className="loading">
        <div className="spinner"></div>
      </div>
    );
  }

  return (
    <div className="dashboard">
      <h1>📈 Advanced Analytics</h1>

      <div style={{ marginBottom: '2rem' }}>
        <button className="btn btn-primary" onClick={() => handleExport('json')} style={{ marginRight: '1rem' }}>
          Export JSON
        </button>
        <button className="btn btn-primary" onClick={() => handleExport('csv')}>
          Export CSV
        </button>
      </div>

      {abnormalityRates && (
        <>
          <div className="chart-card" style={{ marginBottom: '2rem' }}>
            <h2>Abnormality Rates by Gender</h2>
            <div className="stats-grid">
              <div className="stat-card">
                <h3>Male Patients</h3>
                <div className="value">{abnormalityRates.byGender.male.total}</div>
                <div className="change">
                  {abnormalityRates.byGender.male.abnormal} abnormal ({abnormalityRates.byGender.male.rate})
                </div>
              </div>
              <div className="stat-card">
                <h3>Female Patients</h3>
                <div className="value">{abnormalityRates.byGender.female.total}</div>
                <div className="change">
                  {abnormalityRates.byGender.female.abnormal} abnormal ({abnormalityRates.byGender.female.rate})
                </div>
              </div>
              <div className="stat-card">
                <h3>Overall</h3>
                <div className="value">{abnormalityRates.overall.total}</div>
                <div className="change">
                  {abnormalityRates.overall.abnormal} abnormal ({abnormalityRates.overall.rate})
                </div>
              </div>
            </div>
          </div>

          <div className="chart-card" style={{ marginBottom: '2rem' }}>
            <h2>Abnormality Rates by Age Group</h2>
            <div className="patients-table">
              <table>
                <thead>
                  <tr>
                    <th>Age Group</th>
                    <th>Total Patients</th>
                    <th>Abnormal Cases</th>
                    <th>Abnormality Rate</th>
                  </tr>
                </thead>
                <tbody>
                  {Object.entries(abnormalityRates.byAgeGroup).map(([group, data]) => (
                    <tr key={group}>
                      <td><strong>{group}</strong></td>
                      <td>{data.total}</td>
                      <td>{data.abnormal}</td>
                      <td>
                        <span className="badge badge-moderate">{data.rate}</span>
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          </div>
        </>
      )}

      <div className="chart-card">
        <h2>Top 20 High-Risk Patients</h2>
        <div className="patients-table">
          <table>
            <thead>
              <tr>
                <th>Patient ID</th>
                <th>Risk Score</th>
                <th>Risk Category</th>
                <th>Abnormal Probability</th>
              </tr>
            </thead>
            <tbody>
              {highRiskPatients.map((patient) => (
                <tr key={patient.patient_id}>
                  <td><strong>{patient.patient_id}</strong></td>
                  <td><strong style={{ color: '#ef4444' }}>{parseFloat(patient.risk_score || 0).toFixed(2)}</strong></td>
                  <td>
                    <span className="badge badge-high">{patient.risk_category || 'N/A'}</span>
                  </td>
                  <td>{(parseFloat(patient.abnormal_prob || 0) * 100).toFixed(2)}%</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  );
};

export default Analytics;
