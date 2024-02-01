#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <cstring>
#include <cmath>
#include <filesystem>
#include <regex>


static const std::string FISCODEGEN_VERSION = "0.1.4";

struct FuzzySystem {
    std::map<std::string, std::string> systemInfo;
    std::map<std::string, std::map<std::string, std::string>> inputInfo;
    std::map<std::string, std::map<std::string, std::string>> outputInfo;
    std::vector<std::vector<std::string>> ruleInfo;
};

struct ioRange {
    std::string minV;
    std::string maxV;
};

struct mfData {
    std::string tag;
    std::string owner;
    std::string fun;
    std::string points;
};

/*============================================================================*/
template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return std::move(out).str();
}
/*============================================================================*/
std::string removeTrailingZeros(float value)
{
    std::string str = to_string_with_precision( value, 8 );
    str.erase ( str.find_last_not_of('0') + 1, std::string::npos );
    str.erase ( str.find_last_not_of('.') + 1, std::string::npos );
    if (str.find('.') == std::string::npos) {
        str += ".0";
    }
    return str;
}
/*============================================================================*/
std::vector<float> fstringToVector( std::string originalString )
{
    std::vector<float> floatVector;

    std::stringstream ss(originalString);
    float currentFloat;
    while (ss >> currentFloat) {
        floatVector.push_back(currentFloat);

        if (ss.peek() == ' ') {
            ss.ignore();
        }
    }
    return floatVector;
}
/*============================================================================*/
ioRange getRangeFromString( std::string s )
{
    ioRange rOut;
    std::stringstream ss( s );
    double tempMin, tempMax;
    ss.ignore();

    if (ss >> tempMin) {
        rOut.minV =  removeTrailingZeros( tempMin ) + "f";
        if (ss >> tempMax) {
             rOut.maxV =  removeTrailingZeros( tempMax ) + "f";
        }
    }

    return rOut;
}
/*============================================================================*/
std::string trim(const std::string & sStr)
{
    int nSize = sStr.size();
    int nSPos = 0, nEPos = 1, i;
    for(i = 0; i< nSize; ++i) {
        if( !isspace( sStr[i] ) ) {
            nSPos = i ;
            break;
        }
    }
    for(i = nSize -1 ; i >= 0 ; --i) {
        if( !isspace( sStr[i] ) ) {
            nEPos = i;
            break;
        }
    }
    return std::string(sStr, nSPos, nEPos - nSPos + 1);
}
/*============================================================================*/
std::string remContigousDuplicate( std::string originalString, char c )
{
    std::string result;
    char prevChar = '\0';  // Initialize to a character not in the string
    for (char currentChar : originalString) {
        if (currentChar != c || currentChar != prevChar) {
            result += currentChar;
            prevChar = currentChar;
        }
    }
    return result;
}
/*============================================================================*/
void parseFile(const std::string& filename, FuzzySystem& fuzzySystem) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    std::string line;
    std::string currentSection;

    while (std::getline(file, line)) {
        if (line.empty()) { // Skip empty lines
            continue;
        }
        // Check for section headers
        if (line[0] == '[') {
            currentSection = line.substr(1, line.find(']') - 1);
        }
        else {
            // Split key and value
            size_t equalsPos = line.find('=');
            std::string key = trim( line.substr(0, equalsPos) );
            std::string value = trim( line.substr(equalsPos + 1) );

            // Remove single quotes and spaces if present
            value.erase(std::remove(value.begin(), value.end(), '\''), value.end());

            // Store information based on the current section
            if (currentSection == "System") {
                fuzzySystem.systemInfo[key] = value;
            }
            else if (currentSection.find("Input") != std::string::npos) {
                fuzzySystem.inputInfo[currentSection][key] = value;
            }
            else if (currentSection.find("Output") != std::string::npos) {
                fuzzySystem.outputInfo[currentSection][key] = value;
            }
            else if (currentSection == "Rules") {
                std::istringstream ruleStream(value);
                std::vector<std::string> rule;
                std::string token;

                while (std::getline(ruleStream, token, ' ')) {
                    rule.push_back(token);
                }

                fuzzySystem.ruleInfo.push_back(rule);
            }
        }
    }

    file.close();
}
/*============================================================================*/
std::string createVariableName( std::string input) {
    std::string result;

    std::replace(input.begin(), input.end(), '-', '_');
    for (char ch : input) {
        // Replace spaces with underscores
        if (std::isspace(ch)) {
            result.push_back('_');
        }
        // Only include alphanumeric characters and underscores
        else if (std::isalnum(ch) || ch == '_') {
            result.push_back(ch);
        }
    }

    return result;
}
/*============================================================================*/
std::string getMFName( const std::string& str )
{
    // Find the positions of ':' and ','
    size_t colonPos = str.find(':');
    size_t commaPos = str.find(',');
    std::string mfName;
    // Extract the substring between ':' and ','
    if (colonPos != std::string::npos && commaPos != std::string::npos && colonPos < commaPos) {
        mfName = str.substr(colonPos + 1, commaPos - colonPos - 1);
        if ( ( mfName == "linear" ) || ( mfName == "constant" ) ) {
            mfName += "mf";
        }
    }
    else {
        mfName = "unknown";
    }
    return mfName;
}
/*============================================================================*/
std::string parse_points( std::string str )
{
    std::string s;
    std::string p;
    size_t commaPos = str.find(',');

    if (commaPos != std::string::npos && commaPos + 1 < str.size()) {
        s = str.substr(commaPos + 1);
        s.erase(std::remove(s.begin(), s.end(), '['), s.end());
        s.erase(std::remove(s.begin(), s.end(), ']'), s.end());
        s = trim( s );
        auto v = fstringToVector( s );
        p += "{ ";
        for ( auto i : v ) {
            p += removeTrailingZeros( i ) + "f, ";
        }
        p.erase(p.end() - 2, p.end());
        p += " }";
    }
    else {
        p = "{ 0.0f }";
    }

    return p;
}
/*============================================================================*/
std::vector<std::vector<float>> ruleToMatrix( std::string matrixString  )
{
    std::vector<std::vector<float>> matrix;
    const std::string charactersToRemove = "():,";
    matrixString.erase(std::remove_if(matrixString.begin(), matrixString.end(),
                        [&charactersToRemove](char c) {
                            return charactersToRemove.find(c) != std::string::npos;
                        }),
                        matrixString.end());

    std::stringstream ss(matrixString);
    std::string line;
    while (std::getline(ss, line)) {
        std::stringstream lineStream(line);
        std::vector<float> row;
        float value;
        while (lineStream >> value) {
            row.push_back( value  );
        }
        matrix.push_back(row);
    }
    return matrix;
}
/*============================================================================*/
int writeFile( std::string filename, std::string content )
{
    std::ofstream outFile( filename, std::ios::out | std::ios::trunc);

    if (!outFile.is_open()) {
        std::cerr << "Error opening the file." << std::endl;
        return 1;
    }
    outFile << content;
    outFile.close();
    return 0;
}
/*============================================================================*/
std::string strUpper( std::string str )
{
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}
/*============================================================================*/
std::string extractFilenameFromURL(const std::string& url) {
    // Use a regex to extract the part after the last '/'
    std::regex regex("/([^/]+)$");
    std::smatch match;

    if (std::regex_search(url, match, regex)) {
        return match[1];
    } else {
        return ""; // No match found
    }
}
/*============================================================================*/
bool downloadFile( std::string url, std::string destination )
{
    const std::string filename = extractFilenameFromURL(url);
    const std::string outFilename = "\"" + destination + "/" + filename + "\"";
    std::cout << "- Downloading " << filename << " to " << outFilename << std::endl;
    const std::string cmd = "curl -sS -LJO " + url + " --output-dir " + destination;
    //std::cout << cmd << std::endl;
    system( cmd.c_str() );
    return true;
}
/*============================================================================*/
bool createDirectory( const std::string dirPath )
{
    std::filesystem::path existingDirectoryPath = dirPath;

    // Attempt to create the directory
    try {
        std::filesystem::create_directory(existingDirectoryPath);
        std::cout << "Directory created successfully or already exists: " << existingDirectoryPath << std::endl;
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error creating directory: " << e.what() << std::endl;
    }
    return true;
}
/*============================================================================*/
bool removeDirectory( const std::string dirPath )
{
    std::filesystem::path directoryPath = dirPath;

    // Remove the directory
    try {
        std::filesystem::remove(directoryPath);
        std::cout << "Directory removed successfully: " << directoryPath << std::endl;
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error removing directory: " << e.what() << std::endl;
    }
    return true;
}
/*============================================================================*/
int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cout << "fiscodegen : C/C++ code generation from a FIS(Fuzzy Inference System)" << std::endl;
        std::cout << "Version : " + FISCODEGEN_VERSION + "\n" << std::endl;
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1; // Return an error code
    }

    FuzzySystem fis;
    parseFile(argv[1], fis);

    // Access the information stored in fuzzySystem structure as needed
    // For example:
    //std::cout << "System Name: " << fis.systemInfo["Name"] << std::endl;
    //std::cout << "Type: " << fis.systemInfo["Type"] << std::endl;
//
    //auto range = fis.inputInfo["Input1"]["Name"];
//
    //std::cout << "Input1:Name " << fis.inputInfo["Input1"]["Name"] << std::endl;
    //std::cout << "Input1:Range " << fis.inputInfo["Input1"]["Range"] << std::endl;
    //std::cout << "Input1:Range " << fis.inputInfo["Input1"]["MF1"] << std::endl;

    // Access and print the Rules
    std::string strRules;

    //std::cout << "Rules:" << std::endl;
    for (const auto& rule : fis.ruleInfo) {
        for (const auto& token : rule) {
            strRules += token + " ";
        }
        strRules += '\n';
    }
    //std::cout << strRules <<  std::endl;

    auto rMatrix = ruleToMatrix( strRules );


    auto fisVarName = createVariableName( fis.systemInfo["Name"] );

    std::string gen =
    "#include <include/fis.hpp>\n\n"
    "using namespace qlibs;\n\n"
    "\n";

    int nInputs = std::stoi( fis.systemInfo["NumInputs"] );
    int nOutputs = std::stoi( fis.systemInfo["NumOutputs"] );
    int nRules = std::stoi( fis.systemInfo["NumRules"] );
    std::vector<std::string> inputTags;
    std::vector<std::string> outputTags;
    std::vector<std::string> inMFTags;
    std::vector<std::string> outMFTags;
    std::vector<ioRange> inputRanges;
    std::vector<ioRange> outputRanges;
    std::vector<mfData> inMFinfo;
    std::vector<mfData> outMFinfo;

    std::vector<std::vector<std::string>> inTagRelate;
    std::vector<std::vector<std::string>> outTagRelate;


    std::cout << "- Generating from "<< fisVarName <<  " FIS..." << std::endl;

    //std::cout << "nInputs = " << nInputs << std::endl;
    //std::cout << "nOutputs = " << nOutputs << std::endl;
    for ( int i = 1; i <= nInputs ; ++i ) {
        auto in = "Input" + std::to_string(i);
        auto inName = createVariableName( fis.inputInfo[in]["Name"] );
        inputTags.push_back( inName );
        int numMFs = std::stoi( fis.inputInfo[in]["NumMFs"] );

        inputRanges.push_back( getRangeFromString( fis.inputInfo[in]["Range"] ) );
        std::vector<std::string> in_tags;

        for ( int j = 1; j <= numMFs; ++j ) {
            mfData dat;

            auto mfInfo = fis.inputInfo[in]["MF"+std::to_string(j)];
            size_t pos = mfInfo.find(':');
            auto mfName = mfInfo.substr(0, pos);
            inMFTags.push_back( createVariableName( inName + "_" + mfName ) );
            in_tags.push_back( createVariableName( inName + "_" + mfName ) );

            dat.tag = inName + "_" + mfName;
            dat.owner = inName;
            dat.fun = getMFName( mfInfo );
            dat.points = parse_points( mfInfo );
            inMFinfo.push_back( dat );
        }
        inTagRelate.push_back( in_tags );
    }

    for ( int i = 1; i <= nOutputs ; ++i ) {
        auto out = "Output" + std::to_string(i);
        auto outName = createVariableName( fis.outputInfo[out]["Name"] );
        outputTags.push_back( outName );
        int numMFs = std::stoi( fis.outputInfo[out]["NumMFs"] );

        outputRanges.push_back( getRangeFromString( fis.outputInfo[out]["Range"] ) );
        std::vector<std::string> out_tags;

        for ( int j = 1; j <= numMFs; ++j ) {
            mfData dat;

            auto mfInfo = fis.outputInfo[out]["MF"+std::to_string(j)];
            size_t pos = mfInfo.find(':');
            auto mfName = mfInfo.substr(0, pos);
            outMFTags.push_back( createVariableName( outName + "_" + mfName ) );
            out_tags.push_back( createVariableName( outName + "_" + mfName ) );

            dat.tag = outName + "_" + mfName;
            dat.owner = outName;
            dat.fun = getMFName( mfInfo );
            dat.points = parse_points( mfInfo );
            outMFinfo.push_back( dat );
        }
        outTagRelate.push_back( out_tags );
    }

    gen += "/* Tags for I/Os */\n";
    gen += "enum : fis::tag {";
    for ( auto t : inputTags ) {
        gen += " " + t + ",";
    }
    gen.pop_back();
    gen += " };\n";

    gen += "enum : fis::tag {";
    for ( auto t : outputTags ) {
        gen += " " + t + ",";
    }
    gen.pop_back();
    gen += " };\n";

    gen += "/* Tags for I/Os membership functions*/\n";
    gen += "enum : fis::tag {";
    for ( auto t : inMFTags ) {
        gen += " " + t + ",";
    }
    gen.pop_back();
    gen += " };\n";

    gen += "enum : fis::tag {";
    for ( auto t : outMFTags ) {
        gen += " " + t + ",";
    }

    /*generate rules*/
    std::string r;
    for ( int i = 0; i < nRules; ++i ) {
        r += "        IF ";
        std::string operationType = ( 1 == rMatrix[i][ nInputs+nOutputs+1 ] ) ? " AND " : " OR ";
        for ( int j = 0; j < nInputs ; ++j ) {
            int tagIndex = static_cast<int>( rMatrix[ i ][ j ] );
            if ( 0 == tagIndex ) {
                continue;
            }
            std::string neg = "";
            if ( tagIndex < 0 ) {
                neg = "_NOT";
                tagIndex = -tagIndex;
            }
            tagIndex -= 1;
            r += inputTags[ j ] + " IS" + neg + " " + inTagRelate[j][ tagIndex ];
            if ( j != ( nInputs-1 ) && ( ( j < (nInputs-1) ) && ( 0 != static_cast<int>( rMatrix[i][j+1] ) ) )  ) {
                r += operationType;
            }
            else {
                r += " THEN ";
            }
        }
        for ( int j = 0; j < nOutputs ; ++j ) {
            auto tagIndex = static_cast<int>( rMatrix[ i ][ j+nInputs ] );
            if ( 0 == tagIndex ) {
                continue;
            }
            std::string neg = "";
            if ( tagIndex < 0 ) {
                neg = "_NOT";
                tagIndex = -tagIndex;
            }
            tagIndex -= 1;
            r += outputTags[ j ] + " IS" + neg + " " + outTagRelate[j][ tagIndex ];
            if ( j != ( nOutputs-1 ) && ( ( j < (nOutputs-1) ) && ( 0 != static_cast<int>( rMatrix[i][j+nInputs+1] ) ) )  ) {
                r += " AND ";
            }
        }
        r += " END\n";
    }

    gen.pop_back();
    gen += " };\n";

    gen += "/* FIS Instance */\n";
    gen += "static fis::instance " + fisVarName + ";\n";
    gen += "/* Fuzzy I/O objects */\n";
    gen += "static fis::input " + fisVarName + "_inputs[ " + fis.systemInfo["NumInputs"] + " ];\n";
    gen += "static fis::output " + fisVarName + "_outputs[ " + fis.systemInfo["NumOutputs"] + " ];\n";
    gen += "/* Fuzzy I/O Membership Objects */\n";
    gen += "static fis::mf MFin[ " + std::to_string( inMFTags.size() ) + " ], MFout[ " + std::to_string( outMFTags.size() ) + " ];\n";
    gen += "/* Rules for the inference system */\n";
    gen += "static const fis::rules rules[] = {\n";
    gen += "    FIS_RULES_BEGIN\n";
    gen += r;
    gen += "    FIS_RULES_END\n";
    gen += "};\n";
    gen += "/* Rules strengths*/\n";
    gen += "static real_t rStrength[ " + fis.systemInfo["NumRules"] + " ] = { 0.0f };\n\n\n";
    gen += "void " + fisVarName + "Init( void )\n";
    gen += "{\n";

    auto fisType  = fis.systemInfo["Type"];
    fisType[0] = std::toupper(fisType[0]);
    gen += "    " + fisVarName + ".setup( fis::" + fisType + ", " + fisVarName + "_inputs, " + fisVarName + "_outputs, MFin, MFout, rules, rStrength );\n";

    gen += "    " + fisVarName + ".setParameter( fis::FIS_AND, fis::FIS_" + strUpper(fis.systemInfo["AndMethod"]) + " );\n";
    gen += "    " + fisVarName + ".setParameter( fis::FIS_OR, fis::FIS_" + strUpper(fis.systemInfo["OrMethod"]) + " );\n";
    gen += "    " + fisVarName + ".setParameter( fis::FIS_Implication, fis::FIS_" + strUpper(fis.systemInfo["ImpMethod"]) + " );\n";
    gen += "    " + fisVarName + ".setParameter( fis::FIS_Aggregation, fis::FIS_" + strUpper(fis.systemInfo["AggMethod"]) + " );\n";
    gen += "    " + fisVarName + ".setDeFuzzMethod( fis::" + fis.systemInfo["DefuzzMethod"] + " );\n";

    for ( size_t i = 0; i < inputTags.size() ; ++i ) {
        gen += "    " + fisVarName + ".setupInput( " + inputTags[ i ] + ", " +  inputRanges[ i ].minV + ", " + inputRanges[ i ].maxV + " );\n";
    }
    for ( size_t i = 0; i < outputTags.size() ; ++i ) {
        gen += "    " + fisVarName + ".setupOutput( " + outputTags[ i ] + ", " +  outputRanges[ i ].minV + ", " + outputRanges[ i ].maxV + " );\n";
    }

    for ( auto m : inMFinfo ) {
        gen += "    " + fisVarName + ".setupInputMF( " + m.owner + ", " +  m.tag + ", fis::" + m.fun + ", (const real_t[])" + m.points + " );\n";
    }
    for ( auto m : outMFinfo ) {
        gen += "    " + fisVarName + ".setupOutputMF( " + m.owner + ", " +  m.tag + ", fis::" + m.fun + ", (const real_t[])" + m.points + " );\n";
    }

    float sumW = 0.0;
    std::string str_rW = "    " + fisVarName + ".setRuleWeights( (const real_t[]){ ";
    for ( int i = 0; i < nRules; ++i ) {
        sumW += rMatrix[ i ][ nInputs + nOutputs  ];
        str_rW += removeTrailingZeros( rMatrix[ i ][ nInputs + nOutputs  ] ) + ( ( i != (nRules-1) )? "f, " : "f ");
    }
    str_rW += "}\n";

    if ( sumW != nRules ) {
        gen += str_rW;
    }

    //setRuleWeights
    gen += "}\n\n";
    gen += "void " + fisVarName + "Run( real_t *inputs, real_t *outputs )\n";
    gen += "{\n";
    gen += "    /* Set the crips inputs */\n";

    for ( auto t : inputTags ) {
        gen += "    " + fisVarName + ".setInput( " + t + ", inputs[ " + t + " ] );\n";
    }

    gen += "    " + fisVarName + ".fuzzify();\n";
    gen += "    if ( " + fisVarName + ".inference() ) {\n";
    gen += "        " + fisVarName + ".deFuzzify();\n";
    gen += "    }\n";
    gen += "    else {\n";
    gen += "        /* Handle error */\n";
    gen += "    }\n\n";
    gen += "    /* Get the crips outputs */\n";

    for ( auto t : outputTags ) {
        gen += "    outputs[ " + t + " ] = " + fisVarName + "[ " + t + " ];\n";
    }

    gen += "}\n\n";
    //std::cout << "===============================================" << std::endl;
    //std::cout << gen << std::endl;
    std::cout << "- Writing file " << fisVarName+".cpp ..." << std::endl;

    std::filesystem::path currentPath = std::filesystem::current_path();
    std::string srcDestination = currentPath.string() + "/generated";
    std::string incDestination = currentPath.string() + "/generated/include";
    std::string genFileName = srcDestination + "/" + fisVarName + ".cpp";

    createDirectory( srcDestination );
    createDirectory( incDestination );
    std::vector<std::string> fis_src = {
        "https://github.com/kmilo17pet/qlibs-cpp/raw/main/src/fisCore.cpp",
        "https://github.com/kmilo17pet/qlibs-cpp/raw/main/src/fis.cpp",
        "https://github.com/kmilo17pet/qlibs-cpp/raw/main/src/ffmath.cpp"
    };
    std::vector<std::string> fis_inc = {
        "https://github.com/kmilo17pet/qlibs-cpp/raw/main/src/include/qlibs_types.hpp",
        "https://github.com/kmilo17pet/qlibs-cpp/raw/main/src/include/fis.hpp",
        "https://github.com/kmilo17pet/qlibs-cpp/raw/main/src/include/ffmath.hpp"
    };
    writeFile( genFileName, gen );
    std::cout << "- Obtaining qFIS engine from https://github.com/kmilo17pet/qlibs-cpp..." << std::endl;

    for( std::string iFile : fis_src ){
        downloadFile( iFile, "generated" );
    }
    for( std::string iFile : fis_inc ){
        downloadFile( iFile, "generated/include" );
    }

    std::cout << "- Organizing generated files... " <<  currentPath << std::endl;


    //system("rm *.hpp");
    std::cout << "Completed!" << std::endl;
    return 0;
}