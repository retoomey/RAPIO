#define BOOST_TEST_MODULE RandomForestTestSuite
#include <boost/test/included/unit_test.hpp>

#include <rRAPIORuntime.h>
#include <rRandomForest.h>
#include <rError.h>
#include <rOS.h>

#include <fstream>
#include <cmath>

using namespace rapio;

namespace {

void writeTempFile(const std::string& filepath, const std::string& content) {
    std::ofstream ofs(filepath);
    ofs << content;
    ofs.close();
}

} // namespace

// ---------------------------------------------------------------------------
// Test Fixture: Handles RAPIO Runtime Init, Temp File Creation, and Cleanup
// ---------------------------------------------------------------------------
struct RandomForestFixture {
    std::string rfCsvPath;
    std::string impCsvPath;
    std::string rf4CsvPath;

    RandomForestFixture() {
        RAPIORuntime::initialize();

        rfCsvPath  = OS::getUniqueTemporaryFile("test_rf_") + ".csv";
        impCsvPath = OS::getUniqueTemporaryFile("test_imp_") + ".csv";
        rf4CsvPath = OS::getUniqueTemporaryFile("test_rf4_") + ".csv";

        // Model 1: 2-Tree, 2-Feature Forest ("AzShear_max" and "Reflectivity_max")
        std::string rfModelContent = 
            "tableNumber,featureName,leftChild,rightChild,threshold,probability\n"
            "0,AzShear_max,1,2,0.005,0.50\n"
            "0,leaf,-1,-1,0.000,0.10\n"
            "0,leaf,-1,-1,0.000,0.90\n"
            "1,Reflectivity_max,1,2,35.0,0.40\n"
            "1,leaf,-1,-1,0.000,0.20\n"
            "1,leaf,-1,-1,0.000,0.80\n";
        writeTempFile(rfCsvPath, rfModelContent);

        // Baseline Imputation Values
        std::string impContent = 
            "AzShear_max,Reflectivity_max,RhoHV_min,Zdr_max\n"
            "0.002,30.0,0.95,1.5\n";
        writeTempFile(impCsvPath, impContent);

        // Model 2: 4-Feature Forest
        std::string rf4ModelContent = 
            "tableNumber,featureName,leftChild,rightChild,threshold,probability\n"
            "0,AzShear_max,1,2,0.005,0.50\n"
            "0,leaf,-1,-1,0.000,0.10\n"
            "0,leaf,-1,-1,0.000,0.90\n"
            "1,Reflectivity_max,1,2,35.0,0.40\n"
            "1,leaf,-1,-1,0.000,0.20\n"
            "1,leaf,-1,-1,0.000,0.80\n"
            "2,RhoHV_min,1,2,0.90,0.50\n"
            "2,leaf,-1,-1,0.000,0.10\n"
            "2,leaf,-1,-1,0.000,0.90\n"
            "3,Zdr_max,1,2,2.0,0.50\n"
            "3,leaf,-1,-1,0.000,0.10\n"
            "3,leaf,-1,-1,0.000,0.90\n";
        writeTempFile(rf4CsvPath, rf4ModelContent);
    }

    ~RandomForestFixture() {
        OS::deleteFile(rfCsvPath);
        OS::deleteFile(impCsvPath);
        OS::deleteFile(rf4CsvPath);
    }
};

BOOST_FIXTURE_TEST_SUITE(RandomForestTests, RandomForestFixture)

// ---------------------------------------------------------------------------
// Test 1: Model Loading & Feature Alignment
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(test_model_and_imputation_loading) {
    RandomForest rf;
    BOOST_REQUIRE(rf.readForest(rfCsvPath));
    BOOST_REQUIRE(rf.readImputation(impCsvPath));

    BOOST_CHECK_EQUAL(rf.getNumberOfTrees(), 2);
    BOOST_CHECK_EQUAL(rf.getNumFeatures(), 2);
}

// ---------------------------------------------------------------------------
// Test 2: Complete FeatureVector Evaluation (No Missing Data)
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(test_complete_feature_vector_eval) {
    RandomForest rf;
    BOOST_REQUIRE(rf.readForest(rfCsvPath));
    BOOST_REQUIRE(rf.readImputation(impCsvPath));

    FeatureVector fv = rf.createFeatureVector();
    BOOST_REQUIRE(fv.set("AzShear_max", 0.008));
    BOOST_REQUIRE(fv.set("Reflectivity_max", 40.0));

    BOOST_CHECK(fv.hasSufficientData(0.5));
    BOOST_CHECK_EQUAL(fv.getSetCount(), 2);

    ForestProbability prob = rf.getForestProbability(fv);
    // Tree 0 (>0.005) = 0.90, Tree 1 (>35.0) = 0.80 -> Mean = 0.85
    BOOST_CHECK_CLOSE(prob.probability, 0.85, 0.001); // 0.001% tolerance
}

// ---------------------------------------------------------------------------
// Test 3: Fallback Imputation (< 50% Missing Data)
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(test_imputation_fallback) {
    RandomForest rf;
    BOOST_REQUIRE(rf.readForest(rfCsvPath));
    BOOST_REQUIRE(rf.readImputation(impCsvPath));

    FeatureVector fv = rf.createFeatureVector();
    BOOST_REQUIRE(fv.set("AzShear_max", 0.008)); // Set: Tree 0 = 0.90
    // Reflectivity_max is UNSET -> Imputed to 30.0 (<=35.0) -> Tree 1 = 0.20

    BOOST_CHECK(fv.hasSufficientData(0.5));
    BOOST_CHECK_EQUAL(fv.getSetCount(), 1);
    BOOST_CHECK(!fv.isSet(rf.getFeatureIndex("Reflectivity_max")));

    ForestProbability prob = rf.getForestProbability(fv);
    // Mean = (0.90 + 0.20) / 2 = 0.55
    BOOST_CHECK_CLOSE(prob.probability, 0.55, 0.001);
}

// ---------------------------------------------------------------------------
// Test 4: Default Safety Gate Failure (> 50% Missing Data)
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(test_default_safety_gate) {
    RandomForest rf4;
    BOOST_REQUIRE(rf4.readForest(rf4CsvPath));
    BOOST_REQUIRE(rf4.readImputation(impCsvPath));
    BOOST_CHECK_EQUAL(rf4.getNumFeatures(), 4);

    FeatureVector fv = rf4.createFeatureVector();
    BOOST_REQUIRE(fv.set("AzShear_max", 0.008)); // 1 of 4 set = 25% set

    BOOST_CHECK(!fv.hasSufficientData(0.5));
    BOOST_CHECK_EQUAL(fv.getSetCount(), 1);

    ForestProbability prob = rf4.getForestProbability(fv);
    BOOST_CHECK_EQUAL(prob.probability, 0.0);
}

// ---------------------------------------------------------------------------
// Test 5: Configurable Safety Gate Threshold (setMinDataRatio)
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(test_configurable_safety_gate_thresholds) {
    RandomForest rfCustom;
    BOOST_REQUIRE(rfCustom.readForest(rf4CsvPath));
    BOOST_REQUIRE(rfCustom.readImputation(impCsvPath));

    // 5a. Set strict threshold of 75% valid data required
    rfCustom.setMinDataRatio(0.75);
    BOOST_CHECK_CLOSE(rfCustom.getMinDataRatio(), 0.75, 0.001);

    FeatureVector fvStrict = rfCustom.createFeatureVector();
    BOOST_REQUIRE(fvStrict.set("AzShear_max", 0.008));
    BOOST_REQUIRE(fvStrict.set("Reflectivity_max", 40.0));
    // 2 of 4 features set = 50% set. Under 75% strict threshold, this MUST FAIL.
    BOOST_CHECK(!fvStrict.hasSufficientData(rfCustom.getMinDataRatio()));

    ForestProbability probStrictFail = rfCustom.getForestProbability(fvStrict);
    BOOST_CHECK_EQUAL(probStrictFail.probability, 0.0);

    // 5b. Add 3rd feature (3 of 4 set = 75% set). Under 75% threshold, this MUST PASS.
    BOOST_REQUIRE(fvStrict.set("RhoHV_min", 0.95));
    BOOST_CHECK(fvStrict.hasSufficientData(rfCustom.getMinDataRatio()));

    ForestProbability probStrictPass = rfCustom.getForestProbability(fvStrict);
    BOOST_CHECK(probStrictPass.probability > 0.0);

    // 5c. Lower threshold to lax 20% valid data required
    rfCustom.setMinDataRatio(0.20);
    FeatureVector fvLax = rfCustom.createFeatureVector();
    BOOST_REQUIRE(fvLax.set("AzShear_max", 0.008));
    // 1 of 4 features set = 25% set. Under 20% lax threshold, this MUST PASS.
    BOOST_CHECK(fvLax.hasSufficientData(rfCustom.getMinDataRatio()));

    ForestProbability probLaxPass = rfCustom.getForestProbability(fvLax);
    BOOST_CHECK(probLaxPass.probability > 0.0);
}

BOOST_AUTO_TEST_SUITE_END()
