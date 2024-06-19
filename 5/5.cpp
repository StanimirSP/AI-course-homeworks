#include <iostream>
#include <fstream>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <limits>

constexpr unsigned CLASSES = 2,
                   ATTRIBUTES = 16,
                   MAX_ATTRIBUTE_VALUES = 3;

std::ifstream openFileForReading(const char* filename)
{
    std::ifstream ifs(filename);
    using namespace std::string_literals;
    if(!ifs)
        throw std::runtime_error("could not open "s + filename + " for reading");
    return ifs;
}

std::vector<std::string> split(std::string str, char delim)
{
    std::vector<std::string> res;
    for(std::size_t pos = str.rfind(delim); pos != std::string::npos; pos = str.rfind(delim))
    {
        res.push_back(str.substr(pos+1));
        str.erase(pos);
    }
    res.push_back(str);
    std::reverse(res.begin(), res.end());
    return res;
}

class NaiveBayesClassifier
{
    unsigned freq[CLASSES][ATTRIBUTES][MAX_ATTRIBUTE_VALUES] = {},
             classesFreq[CLASSES] = {},
             totalSamples = 0;
    std::string intToClass[CLASSES], intToValue[MAX_ATTRIBUTE_VALUES];
    std::unordered_map<std::string, unsigned> valueToInt, classToInt;
    void addSample(const std::vector<std::string>& sampleLine)
    {
        if(!classToInt.contains(sampleLine[0]))
            intToClass[classToInt[sampleLine[0]] = classToInt.size()] = sampleLine[0];
        unsigned classIndex = classToInt[sampleLine[0]];
        classesFreq[classIndex]++;
        totalSamples++;
        for(unsigned i=1; i<=ATTRIBUTES; i++)
        {
            if(!valueToInt.contains(sampleLine[i]))
                intToValue[valueToInt[sampleLine[i]] = valueToInt.size()] = sampleLine[i];
            freq[classIndex][i-1][valueToInt[sampleLine[i]]]++;
        }
    }
    double calculateProbabilityFor(unsigned classIndex, const std::vector<std::string>& sampleLine) const
    {
        double prob = std::log((double)classesFreq[classIndex]/totalSamples);
        for(unsigned i=0; i<ATTRIBUTES; i++)
        {
            auto valueIndexIterator = valueToInt.find(sampleLine[i]);
            if(valueIndexIterator == valueToInt.end())
                throw std::invalid_argument("invalid sample line: bad attribute value");
            unsigned num = freq[classIndex][i][valueIndexIterator->second], den = classesFreq[classIndex];
            if(!num)
            {
                num++;
                den++;
            }
            prob += std::log((double)num/den);
        }
        return prob;
    }
public:
    // assumes that file format is valid, otherwise the behavior is undefined
    NaiveBayesClassifier(const char* filename, char delim = ',')
    {
        std::ifstream ifs(openFileForReading(filename));
        std::string line;
        while(getline(ifs, line))
            addSample(split(line, delim));
    }
    const std::string& classify(const std::vector<std::string>& sampleLine) const
    {
        if(sampleLine.size() != ATTRIBUTES)
            throw std::invalid_argument("invalid sample line: attribute count mismatch");
        unsigned bestClassInd = 0;
        double bestProbability = -std::numeric_limits<double>::infinity();
        for(unsigned i=0; i<classToInt.size(); i++)
        {
            double prob = calculateProbabilityFor(i, sampleLine);
            if(bestProbability < prob)
            {
                bestProbability = prob;
                bestClassInd = i;
            }
        }
        return intToClass[bestClassInd];
    }
};

int main(int argc, char** argv) try
{
    using namespace std::string_literals;
    if(argc != 3)
        throw std::invalid_argument("Usage: "s + *argv + " <data file> <sample file>");
    NaiveBayesClassifier nbc(argv[1]);
    std::ifstream ifs(openFileForReading(argv[2]));
    std::string line;
    while(getline(ifs, line))
        std::cout << nbc.classify(split(line, ',')) << '\n';
}
catch(const std::exception& e)
{
    std::cerr << e.what() << '\n';
    return 1;
}
