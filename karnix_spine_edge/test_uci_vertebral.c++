/**
 * test_uci_vertebral.c++
 * Test program for UCI Vertebral Column Analysis System
 * 
 * Reads sample dataset and validates classification accuracy
 */

#include "medical/UCIVertebralAnalysis.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>

struct TestCase {
    VertebralFeatures features;
    std::string expected_class;
    std::string patient_id;
};

std::vector<TestCase> load_dataset(const std::string& filename) {
    std::vector<TestCase> cases;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    std::string line;
    int case_num = 0;
    
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> tokens;
        
        while (std::getline(iss, token, ',')) {
            tokens.push_back(token);
        }
        
        if (tokens.size() != 7) continue;
        
        TestCase tc;
        tc.features.pelvic_incidence = std::stod(tokens[0]);
        tc.features.pelvic_tilt = std::stod(tokens[1]);
        tc.features.lumbar_lordosis_angle = std::stod(tokens[2]);
        tc.features.sacral_slope = std::stod(tokens[3]);
        tc.features.pelvic_radius = std::stod(tokens[4]);
        tc.features.grade_of_spondylolisthesis = std::stod(tokens[5]);
        tc.expected_class = tokens[6];
        tc.patient_id = "Patient_" + std::to_string(++case_num);
        
        cases.push_back(tc);
    }
    
    return cases;
}

std::string pathology_to_string(VertebralPathology p) {
    switch (p) {
        case VertebralPathology::NORMAL: return "Normal";
        case VertebralPathology::DISK_HERNIA: return "DiskHernia";
        case VertebralPathology::SPONDYLOLISTHESIS: return "Spondylolisthesis";
    }
    return "Unknown";
}

int main() {
    std::cout << "================================================================================\n";
    std::cout << "  UCI VERTEBRAL COLUMN ANALYSIS - VALIDATION TEST\n";
    std::cout << "================================================================================\n\n";
    
    // Load dataset
    std::cout << "Loading dataset...\n";
    std::vector<TestCase> test_cases;
    
    try {
        test_cases = load_dataset("./data/uci_vertebral_samples.csv");
        std::cout << "✓ Loaded " << test_cases.size() << " test cases\n\n";
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    
    // Initialize analyzer
    UCIVertebralAnalyzer analyzer;
    
    // Test statistics
    int total = 0;
    int correct = 0;
    int normal_cases = 0, hernia_cases = 0, spondy_cases = 0;
    int normal_correct = 0, hernia_correct = 0, spondy_correct = 0;
    
    std::cout << "================================================================================\n";
    std::cout << "  CLASSIFICATION RESULTS\n";
    std::cout << "================================================================================\n\n";
    
    // Process each test case
    for (const auto& tc : test_cases) {
        total++;
        
        // Classify
        VertebralPathology predicted = analyzer.classify(tc.features);
        double risk_score = analyzer.calculate_risk_score(tc.features);
        std::string predicted_str = pathology_to_string(predicted);
        
        bool is_correct = (predicted_str == tc.expected_class);
        if (is_correct) correct++;
        
        // Track per-class accuracy
        if (tc.expected_class == "Normal") {
            normal_cases++;
            if (is_correct) normal_correct++;
        } else if (tc.expected_class == "DiskHernia") {
            hernia_cases++;
            if (is_correct) hernia_correct++;
        } else if (tc.expected_class == "Spondylolisthesis") {
            spondy_cases++;
            if (is_correct) spondy_correct++;
        }
        
        // Print result
        std::cout << std::setw(12) << tc.patient_id << " | ";
        std::cout << "Expected: " << std::setw(18) << tc.expected_class << " | ";
        std::cout << "Predicted: " << std::setw(18) << predicted_str << " | ";
        std::cout << "Risk: " << std::fixed << std::setprecision(1) << std::setw(5) << risk_score << " | ";
        std::cout << (is_correct ? "✓ PASS" : "✗ FAIL") << "\n";
        
        // Show features for failed cases
        if (!is_correct) {
            std::cout << "             Features: PI=" << std::fixed << std::setprecision(1) 
                      << tc.features.pelvic_incidence
                      << "° PT=" << tc.features.pelvic_tilt
                      << "° LL=" << tc.features.lumbar_lordosis_angle
                      << "° SS=" << tc.features.sacral_slope
                      << "° PR=" << tc.features.pelvic_radius
                      << "mm Spondy=" << tc.features.grade_of_spondylolisthesis << "%\n";
        }
    }
    
    // Summary statistics
    std::cout << "\n================================================================================\n";
    std::cout << "  VALIDATION SUMMARY\n";
    std::cout << "================================================================================\n\n";
    
    double overall_accuracy = (total > 0) ? (100.0 * correct / total) : 0.0;
    
    std::cout << "Overall Performance:\n";
    std::cout << "  Total Cases:      " << total << "\n";
    std::cout << "  Correct:          " << correct << "\n";
    std::cout << "  Incorrect:        " << (total - correct) << "\n";
    std::cout << "  Accuracy:         " << std::fixed << std::setprecision(1) 
              << overall_accuracy << "%\n\n";
    
    std::cout << "Per-Class Performance:\n";
    
    if (normal_cases > 0) {
        double normal_acc = 100.0 * normal_correct / normal_cases;
        std::cout << "  Normal:           " << normal_correct << "/" << normal_cases 
                  << " (" << std::fixed << std::setprecision(1) << normal_acc << "%)\n";
    }
    
    if (hernia_cases > 0) {
        double hernia_acc = 100.0 * hernia_correct / hernia_cases;
        std::cout << "  Disk Hernia:      " << hernia_correct << "/" << hernia_cases 
                  << " (" << std::fixed << std::setprecision(1) << hernia_acc << "%)\n";
    }
    
    if (spondy_cases > 0) {
        double spondy_acc = 100.0 * spondy_correct / spondy_cases;
        std::cout << "  Spondylolisthesis:" << spondy_correct << "/" << spondy_cases 
                  << " (" << std::fixed << std::setprecision(1) << spondy_acc << "%)\n";
    }
    
    std::cout << "\n================================================================================\n";
    
    // Detailed analysis of a few cases
    std::cout << "\n  DETAILED FEATURE ANALYSIS (Sample Cases)\n";
    std::cout << "================================================================================\n\n";
    
    for (size_t i = 0; i < std::min(size_t(5), test_cases.size()); i++) {
        const auto& tc = test_cases[i];
        VertebralPathology predicted = analyzer.classify(tc.features);
        double risk_score = analyzer.calculate_risk_score(tc.features);
        
        std::cout << tc.patient_id << " - " << tc.expected_class << "\n";
        std::cout << "  Pelvic Incidence:      " << std::fixed << std::setprecision(2) 
                  << tc.features.pelvic_incidence << "°";
        std::cout << (tc.features.pelvic_incidence > 70.0 ? " [HIGH]" : 
                     tc.features.pelvic_incidence < 40.0 ? " [LOW]" : " [NORMAL]") << "\n";
        
        std::cout << "  Pelvic Tilt:           " << tc.features.pelvic_tilt << "°";
        std::cout << (tc.features.pelvic_tilt > 23.0 ? " [HIGH]" : " [NORMAL]") << "\n";
        
        std::cout << "  Lumbar Lordosis:       " << tc.features.lumbar_lordosis_angle << "°";
        std::cout << (tc.features.lumbar_lordosis_angle < 40.0 ? " [LOW]" : " [NORMAL]") << "\n";
        
        std::cout << "  Sacral Slope:          " << tc.features.sacral_slope << "°";
        std::cout << (tc.features.sacral_slope > 50.0 ? " [HIGH]" : " [NORMAL]") << "\n";
        
        std::cout << "  Pelvic Radius:         " << tc.features.pelvic_radius << " mm\n";
        
        std::cout << "  Spondylolisthesis:     " << tc.features.grade_of_spondylolisthesis << "%";
        std::cout << (tc.features.grade_of_spondylolisthesis > 5.0 ? " [ABNORMAL]" : " [NORMAL]") << "\n";
        
        std::cout << "  → Classification:      " << pathology_to_string(predicted) << "\n";
        std::cout << "  → Risk Score:          " << std::fixed << std::setprecision(1) 
                  << risk_score << "/100\n";
        std::cout << "  → Assessment:          " << analyzer.get_risk_assessment(risk_score, predicted) << "\n\n";
    }
    
    std::cout << "================================================================================\n";
    
    if (overall_accuracy >= 70.0) {
        std::cout << "\n✓ VALIDATION PASSED - Classifier performance acceptable (≥70%)\n\n";
        return 0;
    } else {
        std::cout << "\n✗ VALIDATION FAILED - Classifier performance below threshold (<70%)\n\n";
        return 1;
    }
}
