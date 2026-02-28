import React, { useState, useEffect } from 'react';
import { patientAPI } from '../services/api';

const Patients = () => {
  const [patients, setPatients] = useState([]);
  const [loading, setLoading] = useState(true);
  const [search, setSearch] = useState('');
  const [filter, setFilter] = useState('all');

  useEffect(() => {
    loadPatients();
  }, [filter]);

  const loadPatients = async () => {
    try {
      setLoading(true);
      let response;
      
      if (filter === 'abnormal') {
        response = await patientAPI.getAbnormal();
        setPatients(response.data.data);
      } else if (filter === 'high') {
        response = await patientAPI.getAll({ riskCategory: 'HIGH' });
        setPatients(response.data.data);
      } else if (filter === 'moderate') {
        response = await patientAPI.getAll({ riskCategory: 'MODERATE' });
        setPatients(response.data.data);
      } else if (filter === 'low') {
        response = await patientAPI.getAll({ riskCategory: 'LOW' });
        setPatients(response.data.data);
      } else {
        response = await patientAPI.getAll({});
        setPatients(response.data.data);
      }
    } catch (error) {
      console.error('Failed to load patients:', error);
    } finally {
      setLoading(false);
    }
  };

  const handleSearch = async () => {
    if (!search.trim()) {
      loadPatients();
      return;
    }

    try {
      setLoading(true);
      const response = await patientAPI.search(search);
      setPatients(response.data.data);
    } catch (error) {
      console.error('Search failed:', error);
    } finally {
      setLoading(false);
    }
  };

  const filteredPatients = patients.filter(patient => 
    patient.patient_id.toLowerCase().includes(search.toLowerCase()) ||
    patient.classification.toLowerCase().includes(search.toLowerCase())
  );

  return (
    <div className="patients-container">
      <div className="patients-header">
        <h1>👥 Patients</h1>
        <button className="btn btn-primary">+ Add Patient</button>
      </div>

      <div className="search-bar">
        <input
          type="text"
          className="search-input"
          placeholder="Search by Patient ID or Classification..."
          value={search}
          onChange={(e) => setSearch(e.target.value)}
          onKeyPress={(e) => e.key === 'Enter' && handleSearch()}
        />
        <button className="btn btn-primary" onClick={handleSearch}>
          Search
        </button>
      </div>

      <div style={{ marginBottom: '1rem', display: 'flex', gap: '1rem' }}>
        <button 
          className={`btn ${filter === 'all' ? 'btn-primary' : 'btn-secondary'}`}
          onClick={() => setFilter('all')}
        >
          All
        </button>
        <button 
          className={`btn ${filter === 'abnormal' ? 'btn-primary' : 'btn-secondary'}`}
          onClick={() => setFilter('abnormal')}
        >
          Abnormal
        </button>
        <button 
          className={`btn ${filter === 'high' ? 'btn-primary' : 'btn-secondary'}`}
          onClick={() => setFilter('high')}
        >
          High Risk
        </button>
        <button 
          className={`btn ${filter === 'moderate' ? 'btn-primary' : 'btn-secondary'}`}
          onClick={() => setFilter('moderate')}
        >
          Moderate
        </button>
        <button 
          className={`btn ${filter === 'low' ? 'btn-primary' : 'btn-secondary'}`}
          onClick={() => setFilter('low')}
        >
          Low Risk
        </button>
      </div>

      {loading ? (
        <div className="loading">
          <div className="spinner"></div>
        </div>
      ) : (
        <div className="patients-table">
          <table>
            <thead>
              <tr>
                <th>Patient ID</th>
                <th>Age</th>
                <th>Sex</th>
                <th>Risk Score</th>
                <th>Risk Category</th>
                <th>Abnormal Prob</th>
                <th>Classification</th>
                <th>Scan Date</th>
              </tr>
            </thead>
            <tbody>
              {filteredPatients.length === 0 ? (
                <tr>
                  <td colSpan="8" style={{ textAlign: 'center', padding: '2rem' }}>
                    No patients found
                  </td>
                </tr>
              ) : (
                filteredPatients.map((patient) => (
                  <tr key={patient.id}>
                    <td><strong>{patient.patient_id}</strong></td>
                    <td>{patient.age || 'N/A'}</td>
                    <td>{patient.sex || 'N/A'}</td>
                    <td><strong>{parseFloat(patient.risk_score || 0).toFixed(2)}</strong></td>
                    <td>
                      <span className={`badge badge-${(patient.risk_category || 'low').toLowerCase()}`}>
                        {patient.risk_category || 'N/A'}
                      </span>
                    </td>
                    <td>{(parseFloat(patient.abnormal_prob || 0) * 100).toFixed(2)}%</td>
                    <td>
                      <span className={`badge badge-${(patient.dl_classification || patient.classification || 'normal').toLowerCase()}`}>
                        {patient.dl_classification || patient.classification || 'N/A'}
                      </span>
                    </td>
                    <td>{new Date(patient.scan_date || patient.date).toLocaleDateString()}</td>
                  </tr>
                ))
              )}
            </tbody>
          </table>
        </div>
      )}

      <div style={{ marginTop: '1rem', color: 'white', textAlign: 'center' }}>
        Showing {filteredPatients.length} patient(s)
      </div>
    </div>
  );
};

export default Patients;
