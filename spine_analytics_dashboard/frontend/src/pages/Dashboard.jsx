import React, { useState, useEffect } from 'react';
import { analyticsAPI } from '../services/api';
import { 
  Chart as ChartJS, 
  CategoryScale, 
  LinearScale, 
  BarElement, 
  Title, 
  Tooltip, 
  Legend,
  ArcElement,
  PointElement,
  LineElement
} from 'chart.js';
import { Bar, Doughnut, Line } from 'react-chartjs-2';

ChartJS.register(
  CategoryScale,
  LinearScale,
  BarElement,
  ArcElement,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend
);

const Dashboard = () => {
  const [stats, setStats] = useState(null);
  const [riskDist, setRiskDist] = useState(null);
  const [trends, setTrends] = useState(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    loadDashboardData();
  }, []);

  const loadDashboardData = async () => {
    try {
      const [statsRes, riskRes, trendsRes] = await Promise.all([
        analyticsAPI.getDashboard(),
        analyticsAPI.getRiskDistribution(),
        analyticsAPI.getTrends(30)
      ]);

      setStats(statsRes.data.data);
      setRiskDist(riskRes.data.data);
      setTrends(trendsRes.data.data);
    } catch (error) {
      console.error('Failed to load dashboard data:', error);
    } finally {
      setLoading(false);
    }
  };

  if (loading) {
    return (
      <div className="loading">
        <div className="spinner"></div>
      </div>
    );
  }

  if (!stats) {
    return <div className="dashboard"><h1>Failed to load dashboard data</h1></div>;
  }

  // Risk Distribution Chart Data
  const riskChartData = {
    labels: ['High Risk', 'Moderate Risk', 'Low Risk'],
    datasets: [
      {
        label: 'Number of Patients',
        data: [riskDist?.high || 0, riskDist?.moderate || 0, riskDist?.low || 0],
        backgroundColor: [
          'rgba(239, 68, 68, 0.8)',
          'rgba(251, 191, 36, 0.8)',
          'rgba(34, 197, 94, 0.8)'
        ],
        borderColor: [
          'rgba(239, 68, 68, 1)',
          'rgba(251, 191, 36, 1)',
          'rgba(34, 197, 94, 1)'
        ],
        borderWidth: 2
      }
    ]
  };

  // Gender Distribution Chart
  const genderChartData = {
    labels: ['Male', 'Female'],
    datasets: [
      {
        label: 'Patients by Gender',
        data: [stats?.demographics?.male || 0, stats?.demographics?.female || 0],
        backgroundColor: [
          'rgba(59, 130, 246, 0.8)',
          'rgba(236, 72, 153, 0.8)'
        ],
        borderColor: [
          'rgba(59, 130, 246, 1)',
          'rgba(236, 72, 153, 1)'
        ],
        borderWidth: 2
      }
    ]
  };

  // Trends Chart Data
  const trendsChartData = trends ? {
    labels: trends.map(t => new Date(t.date).toLocaleDateString()),
    datasets: [
      {
        label: 'Daily Scans',
        data: trends.map(t => t.total_scans || t.scanCount || 0),
        borderColor: 'rgba(102, 126, 234, 1)',
        backgroundColor: 'rgba(102, 126, 234, 0.1)',
        tension: 0.4
      },
      {
        label: 'Abnormal Cases',
        data: trends.map(t => t.abnormal_cases || t.abnormalCount || 0),
        borderColor: 'rgba(239, 68, 68, 1)',
        backgroundColor: 'rgba(239, 68, 68, 0.1)',
        tension: 0.4
      }
    ]
  } : null;

  return (
    <div className="dashboard">
      <h1>📊 Dashboard Overview</h1>

      <div className="stats-grid">
        <div className="stat-card">
          <h3>Total Patients</h3>
          <div className="value">{stats.overview.totalPatients}</div>
          <div className="change">All time</div>
        </div>

        <div className="stat-card">
          <h3>Abnormal Cases</h3>
          <div className="value">{stats.overview.abnormalCases}</div>
          <div className="change">{stats.overview.abnormalRate}</div>
        </div>

        <div className="stat-card">
          <h3>High Risk</h3>
          <div className="value" style={{ color: '#ef4444' }}>
            {riskDist?.high || 0}
          </div>
          <div className="change">Require attention</div>
        </div>

        <div className="stat-card">
          <h3>Recent Scans</h3>
          <div className="value">{stats.overview.recentScans}</div>
          <div className="change">Last 30 days</div>
        </div>
      </div>

      <div className="charts-grid">
        <div className="chart-card">
          <h2>Risk Distribution</h2>
          <Bar 
            data={riskChartData} 
            options={{ 
              responsive: true,
              plugins: {
                legend: { display: false }
              }
            }} 
          />
        </div>

        <div className="chart-card">
          <h2>Gender Distribution</h2>
          <Doughnut 
            data={genderChartData}
            options={{ 
              responsive: true,
              plugins: {
                legend: { position: 'bottom' }
              }
            }} 
          />
        </div>
      </div>

      {trendsChartData && (
        <div className="chart-card" style={{ marginBottom: '2rem' }}>
          <h2>Trends (Last 30 Days)</h2>
          <Line 
            data={trendsChartData}
            options={{
              responsive: true,
              plugins: {
                legend: { position: 'top' }
              },
              scales: {
                y: { beginAtZero: true }
              }
            }}
          />
        </div>
      )}

      <div className="stats-grid">
        <div className="stat-card">
          <h3>Average Age</h3>
          <div className="value">{stats.ageStatistics.average}</div>
          <div className="change">Range: {stats.ageStatistics.minimum} - {stats.ageStatistics.maximum}</div>
        </div>

        <div className="stat-card">
          <h3>Avg Risk Score</h3>
          <div className="value">{stats.riskScoreStatistics.average}</div>
          <div className="change">Out of 100</div>
        </div>

        <div className="stat-card">
          <h3>Male Patients</h3>
          <div className="value">{stats.demographics.male}</div>
          <div className="change">{stats.demographics.malePercentage}</div>
        </div>

        <div className="stat-card">
          <h3>Female Patients</h3>
          <div className="value">{stats.demographics.female}</div>
          <div className="change">{stats.demographics.femalePercentage}</div>
        </div>
      </div>
    </div>
  );
};

export default Dashboard;
