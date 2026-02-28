/**
 * ClinicalScoring.hpp
 * Rule-based clinical scoring system for spine assessment
 * 
 * Implements:
 * - Cobb Angle measurement and scoliosis grading
 * - Vertebral wedging analysis (compression fractures)
 * - FRAX-based fracture risk assessment
 * - WHO osteoporosis classification
 * - Spinal instability score
 */

#ifndef CLINICAL_SCORING_HPP
#define CLINICAL_SCORING_HPP

#include <cmath>
#include <string>
#include <vector>
#include <algorithm>

/**
 * Clinical measurements from imaging
 */
struct ClinicalMeasurements {
    // Spine curvature
    double cobb_angle = 0.0;              // degrees (0-90+)
    double kyphosis_angle = 0.0;          // thoracic curvature, degrees
    double lordosis_angle = 0.0;          // lumbar curvature, degrees
    
    // Vertebral body measurements
    double anterior_height = 0.0;         // mm
    double posterior_height = 0.0;        // mm
    double middle_height = 0.0;           // mm
    
    // Disc spacing
    double disc_height = 0.0;             // mm
    double disc_degeneration_grade = 0;   // Pfirrmann grade 1-5
    
    // Bone density (if available from DEXA or CT)
    double bone_mineral_density = 0.0;    // g/cm²
    double t_score = 0.0;                 // standard deviations
    
    // Alignment
    double vertebral_slip = 0.0;          // mm (spondylolisthesis)
    double spinal_canal_stenosis = 0.0;   // percentage narrowing
    
    // Patient demographics (for fracture risk)
    int age = 0;
    bool female = false;
    double weight_kg = 70.0;
    double height_cm = 170.0;
    
    // Risk factors
    bool prior_fracture = false;
    bool parent_fractured_hip = false;
    bool current_smoking = false;
    bool glucocorticoids = false;
    bool rheumatoid_arthritis = false;
    bool secondary_osteoporosis = false;
    
    // Estimate from image features
    static ClinicalMeasurements estimate_from_image(
        double mean_intensity, double variance, double edge_score,
        double texture_complexity) {
        
        ClinicalMeasurements cm;
        
        // Cobb angle estimation (0 = normal, >10 = scoliosis)
        cm.cobb_angle = edge_score * 45.0;  // 0-45 degrees
        
        // Kyphosis (normal: 20-45°)
        cm.kyphosis_angle = 25.0 + variance * 30.0;
        cm.kyphosis_angle = std::max(10.0, std::min(70.0, cm.kyphosis_angle));
        
        // Lordosis (normal: 40-60°)
        cm.lordosis_angle = 45.0 + mean_intensity * 25.0;
        cm.lordosis_angle = std::max(30.0, std::min(75.0, cm.lordosis_angle));
        
        // Vertebral heights (compression fracture if anterior < 0.8 * posterior)
        cm.posterior_height = 25.0;  // mm (typical)
        cm.anterior_height = 25.0 - (variance * 10.0);  // reduction indicates fracture
        cm.middle_height = (cm.anterior_height + cm.posterior_height) / 2.0;
        
        // Disc height (normal: 7-10mm, degeneration < 5mm)
        cm.disc_height = 8.0 - (texture_complexity * 4.0);
        cm.disc_height = std::max(3.0, std::min(10.0, cm.disc_height));
        
        // Disc degeneration grade (Pfirrmann 1-5)
        if (cm.disc_height > 7.5) cm.disc_degeneration_grade = 1;
        else if (cm.disc_height > 6.5) cm.disc_degeneration_grade = 2;
        else if (cm.disc_height > 5.5) cm.disc_degeneration_grade = 3;
        else if (cm.disc_height > 4.5) cm.disc_degeneration_grade = 4;
        else cm.disc_degeneration_grade = 5;
        
        // Vertebral slip (spondylolisthesis)
        cm.vertebral_slip = edge_score * 8.0;  // 0-8mm
        
        // Spinal canal stenosis
        cm.spinal_canal_stenosis = texture_complexity * 60.0;
        
        // T-score estimation (normal: > -1.0, osteopenia: -1.0 to -2.5, osteoporosis: < -2.5)
        cm.t_score = 0.5 - variance * 4.0 - edge_score * 2.0;
        cm.t_score = std::max(-4.0, std::min(2.0, cm.t_score));
        
        return cm;
    }
};

/**
 * Scoliosis severity classification
 */
enum class ScoliosisSeverity {
    NONE,           // < 10°
    MILD,           // 10-25°
    MODERATE,       // 25-40°
    SEVERE,         // 40-50°
    VERY_SEVERE     // > 50°
};

/**
 * Fracture risk category
 */
enum class FractureRisk {
    LOW,
    MODERATE,
    HIGH,
    VERY_HIGH
};

/**
 * Clinical Scoring System
 */
class ClinicalScoringSystem {
public:
    /**
     * Cobb angle classification (scoliosis assessment)
     */
    static ScoliosisSeverity classify_scoliosis(double cobb_angle) {
        if (cobb_angle < 10.0) return ScoliosisSeverity::NONE;
        if (cobb_angle < 25.0) return ScoliosisSeverity::MILD;
        if (cobb_angle < 40.0) return ScoliosisSeverity::MODERATE;
        if (cobb_angle < 50.0) return ScoliosisSeverity::SEVERE;
        return ScoliosisSeverity::VERY_SEVERE;
    }
    
    static std::string scoliosis_name(ScoliosisSeverity severity) {
        switch (severity) {
            case ScoliosisSeverity::NONE: return "None";
            case ScoliosisSeverity::MILD: return "Mild";
            case ScoliosisSeverity::MODERATE: return "Moderate";
            case ScoliosisSeverity::SEVERE: return "Severe";
            case ScoliosisSeverity::VERY_SEVERE: return "Very Severe";
        }
        return "Unknown";
    }
    
    /**
     * Vertebral compression fracture assessment (wedging)
     */
    static bool has_compression_fracture(const ClinicalMeasurements& cm) {
        // Genant semi-quantitative method
        // Fracture if anterior height < 80% of posterior height
        double height_ratio = cm.anterior_height / cm.posterior_height;
        return height_ratio < 0.80;
    }
    
    static int compression_fracture_grade(const ClinicalMeasurements& cm) {
        double height_ratio = cm.anterior_height / cm.posterior_height;
        
        if (height_ratio >= 0.80) return 0;      // Normal
        if (height_ratio >= 0.75) return 1;      // Mild (20-25% reduction)
        if (height_ratio >= 0.60) return 2;      // Moderate (25-40% reduction)
        return 3;                                 // Severe (>40% reduction)
    }
    
    /**
     * WHO Osteoporosis Classification based on T-score
     */
    static std::string classify_bone_density(double t_score) {
        if (t_score >= -1.0) return "Normal";
        if (t_score >= -2.5) return "Osteopenia";
        return "Osteoporosis";
    }
    
    /**
     * FRAX-inspired 10-year fracture risk calculation
     * Simplified version without full FRAX algorithm
     */
    static double calculate_fracture_risk_score(const ClinicalMeasurements& cm) {
        double risk = 0.0;
        
        // Age factor (exponential increase after 50)
        if (cm.age < 50) {
            risk += 1.0;
        } else {
            risk += 1.0 + std::pow((cm.age - 50) / 10.0, 1.5) * 3.0;
        }
        
        // Sex factor
        if (cm.female) risk *= 1.5;
        
        // BMD/T-score factor (most important)
        if (cm.t_score < -2.5) {
            risk += 8.0;  // Osteoporosis
        } else if (cm.t_score < -1.0) {
            risk += 4.0;  // Osteopenia
        } else {
            risk += 1.0;  // Normal
        }
        
        // Clinical risk factors
        if (cm.prior_fracture) risk += 3.0;
        if (cm.parent_fractured_hip) risk += 2.0;
        if (cm.current_smoking) risk += 1.5;
        if (cm.glucocorticoids) risk += 2.5;
        if (cm.rheumatoid_arthritis) risk += 1.5;
        if (cm.secondary_osteoporosis) risk += 2.0;
        
        // BMI factor (low BMI increases risk)
        double bmi = cm.weight_kg / std::pow(cm.height_cm / 100.0, 2);
        if (bmi < 20.0) risk += 2.0;
        else if (bmi < 25.0) risk += 0.5;
        
        // Convert to percentage (calibrated to typical FRAX outputs)
        return std::min(100.0, risk * 3.0);
    }
    
    static FractureRisk classify_fracture_risk(double risk_percentage) {
        if (risk_percentage < 10.0) return FractureRisk::LOW;
        if (risk_percentage < 20.0) return FractureRisk::MODERATE;
        if (risk_percentage < 30.0) return FractureRisk::HIGH;
        return FractureRisk::VERY_HIGH;
    }
    
    static std::string fracture_risk_name(FractureRisk risk) {
        switch (risk) {
            case FractureRisk::LOW: return "Low";
            case FractureRisk::MODERATE: return "Moderate";
            case FractureRisk::HIGH: return "High";
            case FractureRisk::VERY_HIGH: return "Very High";
        }
        return "Unknown";
    }
    
    /**
     * Disc degeneration assessment (Pfirrmann grading)
     */
    static std::string disc_degeneration_grade_name(int grade) {
        switch (grade) {
            case 1: return "Grade I - Normal";
            case 2: return "Grade II - Minimal degeneration";
            case 3: return "Grade III - Moderate degeneration";
            case 4: return "Grade IV - Severe degeneration";
            case 5: return "Grade V - Advanced degeneration";
            default: return "Unknown";
        }
    }
    
    /**
     * Spinal instability score (White-Panjabi criteria inspired)
     */
    static int calculate_instability_score(const ClinicalMeasurements& cm) {
        int score = 0;
        
        // Vertebral slip > 3mm
        if (cm.vertebral_slip > 3.0) score += 2;
        
        // Excessive disc height loss
        if (cm.disc_height < 5.0) score += 2;
        
        // Severe disc degeneration
        if (cm.disc_degeneration_grade >= 4) score += 2;
        
        // Excessive kyphosis (>50°)
        if (cm.kyphosis_angle > 50.0) score += 2;
        
        // Compression fracture
        if (cm.anterior_height < 0.75 * cm.posterior_height) score += 3;
        
        // Canal stenosis
        if (cm.spinal_canal_stenosis > 50.0) score += 2;
        
        // Score > 5 suggests instability requiring intervention
        return score;
    }
    
    /**
     * Comprehensive clinical assessment
     */
    static std::string generate_clinical_report(const ClinicalMeasurements& cm) {
        std::string report;
        
        // Scoliosis
        auto scoliosis = classify_scoliosis(cm.cobb_angle);
        report += "Scoliosis: " + scoliosis_name(scoliosis);
        if (scoliosis != ScoliosisSeverity::NONE) {
            report += " (Cobb angle: " + std::to_string(static_cast<int>(cm.cobb_angle)) + "°)";
        }
        report += "\n";
        
        // Compression fracture
        int fracture_grade = compression_fracture_grade(cm);
        if (fracture_grade > 0) {
            report += "Compression Fracture: Grade " + std::to_string(fracture_grade);
            report += " (" + std::to_string(static_cast<int>((1.0 - cm.anterior_height/cm.posterior_height) * 100)) + "% height loss)\n";
        } else {
            report += "Compression Fracture: None detected\n";
        }
        
        // Bone density
        report += "Bone Density: " + classify_bone_density(cm.t_score);
        report += " (T-score: " + std::to_string(cm.t_score).substr(0, 4) + ")\n";
        
        // Disc degeneration
        report += "Disc Degeneration: " + disc_degeneration_grade_name(cm.disc_degeneration_grade) + "\n";
        
        // Fracture risk
        double fracture_risk_pct = calculate_fracture_risk_score(cm);
        auto fracture_risk_cat = classify_fracture_risk(fracture_risk_pct);
        report += "10-Year Fracture Risk: " + fracture_risk_name(fracture_risk_cat);
        report += " (" + std::to_string(static_cast<int>(fracture_risk_pct)) + "%)\n";
        
        // Instability
        int instability = calculate_instability_score(cm);
        report += "Spinal Instability Score: " + std::to_string(instability);
        if (instability >= 5) {
            report += " (Surgical evaluation recommended)";
        }
        report += "\n";
        
        return report;
    }
    
    /**
     * Calculate overall clinical severity score (0-100)
     */
    static double calculate_clinical_severity(const ClinicalMeasurements& cm) {
        double severity = 0.0;
        
        // Scoliosis contribution (20%)
        severity += (std::min(50.0, cm.cobb_angle) / 50.0) * 20.0;
        
        // Fracture contribution (25%)
        int fracture_grade = compression_fracture_grade(cm);
        severity += (fracture_grade / 3.0) * 25.0;
        
        // Bone density contribution (20%)
        if (cm.t_score < -2.5) severity += 20.0;
        else if (cm.t_score < -1.0) severity += 10.0;
        
        // Disc degeneration contribution (15%)
        severity += ((cm.disc_degeneration_grade - 1) / 4.0) * 15.0;
        
        // Instability contribution (20%)
        int instability = calculate_instability_score(cm);
        severity += (std::min(10, instability) / 10.0) * 20.0;
        
        return std::min(100.0, severity);
    }
};

#endif // CLINICAL_SCORING_HPP
