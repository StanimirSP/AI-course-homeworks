#include <iostream>
#include <fstream>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <limits>
#include <memory>

const std::unordered_map<std::string, unsigned> CLASSES_STR_TO_INT = {{"recurrence-events", 0}, {"no-recurrence-events", 1}};
const std::string CLASSES_INT_TO_STR[] = {"recurrence-events", "no-recurrence-events"};
constexpr unsigned CLASSES_CNT = 2,
                   ATTRIBUTES = 9,
                   K = 0,
                   MAX_TREE_LEVELS = 3;
typedef std::vector<std::string> Record;

std::ifstream openFileForReading(const char* filename)
{
    std::ifstream ifs(filename);
    if(!ifs)
        throw std::runtime_error(std::string("could not open ") + filename + " for reading");
    return ifs;
}

Record split(std::string str, char delim)
{
    Record res;
    for(std::size_t pos = str.rfind(delim); pos != std::string::npos; pos = str.rfind(delim))
    {
        res.push_back(str.substr(pos+1));
        str.erase(pos);
    }
    res.push_back(str);
    std::reverse(res.begin(), res.end());
    return res;
}

double log2(double arg)
{
    return std::log(arg)/std::log(2);
}

double entropy(const std::vector<double>& probabilities)
{
    double res = 0;
    for(double x: probabilities)
        if(x>0)
            res -= x*log2(x);
    return res;
}

bool fpCompareEqual(double a, double b, double eps = 0.00001)
{
    return std::abs(a-b) < eps;
}

class DatasetView
{
    friend class Dataset;
    double targetEntropy;
    std::vector<const Record*> view;
    double entropy(unsigned attrInd) const
    {
        double res = 0;
        for(const std::string& attrVal: (*attrValues)[attrInd])
        {
            DatasetView temp = filter(attrInd, attrVal);
            res += temp.entropy()*temp.size()/size();
        }
        return res;
    }
    double gain(unsigned attrInd) const
    {
        return targetEntropy - entropy(attrInd);
    }
public:
    const std::vector<std::unordered_set<std::string>> *attrValues;
    std::size_t size() const
    {
        return view.size();
    }
    const Record& operator[](std::size_t n) const
    {
        return *view[n];
    }
    double entropy() const
    {
        std::vector<double> probabilities(CLASSES_CNT);
        for(const Record* r: view)
            probabilities[CLASSES_STR_TO_INT.find((*r)[0])->second]++;
        for(double& p: probabilities)
            p /= view.size();
        return ::entropy(probabilities);
    }
    DatasetView filter(std::size_t attribute, const std::string& value) const
    {
        DatasetView dv;
        dv.targetEntropy = targetEntropy;
        dv.attrValues = attrValues;
        for(const Record* r: view)
        {
            if((*r)[attribute] == value)
                dv.view.push_back(r);
        }
        return dv;
    }
    int getBestUnusedAttribute(const std::vector<bool>& attributeUsed) const
    {
        double bestGain = -std::numeric_limits<double>::infinity();
        int bestInd = -1;
        for(unsigned i=1; i<=ATTRIBUTES; i++)
            if(!attributeUsed[i])
                if(double g = gain(i); g > bestGain)
                {
                    bestGain = g;
                    bestInd = i;
                }
        return bestInd;
    }
    unsigned mostCommonClass() const
    {
        unsigned classesAppearanceCnt[CLASSES_CNT] = {};
        for(const Record* r: view)
            classesAppearanceCnt[CLASSES_STR_TO_INT.find((*r)[0])->second]++;
        return std::max_element(classesAppearanceCnt, classesAppearanceCnt+CLASSES_CNT) - classesAppearanceCnt;
    }
};

class Dataset
{
    std::vector<Record> table;
    std::vector<std::unordered_set<std::string>> attrValues;
public:
    Dataset(const char* filename, char delim = ','): attrValues(ATTRIBUTES+1)
    {
        std::ifstream ifs(openFileForReading(filename));
        std::string line;
        while(getline(ifs, line))
        {
            table.push_back(split(line, delim));
            for(std::size_t i=0; i<=ATTRIBUTES; i++)
                attrValues[i].insert(table.back()[i]);
        }
    }
    DatasetView getView() const
    {
        DatasetView dv;
        dv.attrValues = &attrValues;
        for(const Record& r: table)
            dv.view.push_back(&r);
        dv.targetEntropy = dv.entropy();
        return dv;
    }
    const std::unordered_set<std::string>& getAttrValues(unsigned attrInd) const
    {
        return attrValues[attrInd];
    }
};

class DecisionTree
{
    int attrInd;
    std::unordered_map<std::string, std::unique_ptr<DecisionTree>> children;
    bool isLeaf() const
    {
        return children.empty();
    }
    DecisionTree(const DatasetView& dsv, std::vector<bool>&& attributeUsed, unsigned level)
    {
        if(dsv.size() < K || level >= MAX_TREE_LEVELS || fpCompareEqual(dsv.entropy(), 0) || (attrInd = dsv.getBestUnusedAttribute(attributeUsed)) < 0)
        {
            attrInd = dsv.mostCommonClass();
            return;
        }
        for(const std::string& attrVal: (*dsv.attrValues)[attrInd])
        {
            attributeUsed[attrInd] = true;
            children.emplace(attrVal, new DecisionTree(dsv.filter(attrInd, attrVal), std::move(attributeUsed), level+1));
            attributeUsed[attrInd] = false;
        }
    }
public:
    DecisionTree(const Dataset& ds): DecisionTree(ds.getView(), std::vector<bool>(ATTRIBUTES), 0) {}
    const std::string& classify(const Record& sampleLine) const
    {
        if(isLeaf()) return CLASSES_INT_TO_STR[attrInd];
        auto nextBranchIt = children.find(sampleLine[attrInd-1]);
        if(nextBranchIt == children.end())
        {
            //throw std::invalid_argument(std::string("invalid sample line: bad attribute value: ") + sampleLine[attrInd-1]);
            nextBranchIt = children.begin();
        }
        return nextBranchIt->second->classify(sampleLine);
    }
};

int main(int argc, char** argv) try
{
    if(argc != 3)
        throw std::invalid_argument(std::string("Usage: ") + *argv + " <data file> <sample file>");
    DecisionTree tree{Dataset(argv[1])};
    std::ifstream ifs(openFileForReading(argv[2]));
    std::string line;
    while(getline(ifs, line))
        std::cout << tree.classify(split(line, ',')) << '\n';
}
catch(const std::exception& e)
{
    std::cerr << e.what() << '\n';
    return 1;
}
