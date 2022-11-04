#include <iostream>
#include <list>
#include <cmath>
#include <vector>
#include <memory>
using namespace std;

uint16_t kUpdateMarker = 1u << 15;
int kValueCount = 32768;

float Odds(float probability)
{
    return probability / (1.f - probability);
}

float ProbabilityFromOdds(const float odds)
{
    return odds / (odds + 1.f);
}

float ProbabilityToCorrespondenceCost(const float probability)
{
    return 1.f - probability;
}

float CorrespondenceCostToProbability(const float correspondence_cost)
{
    return 1.f - correspondence_cost;
}

float SlowValueToBoundedFloat(const uint16_t value, const uint16_t unknown_value,
                              const float unknown_result,
                              const float lower_bound,
                              const float upper_bound)
{
    if (value == unknown_value)
        return unknown_result;
    const float kScale = (upper_bound - lower_bound) / (kValueCount - 2.f);
    return value * kScale + (lower_bound - kScale);
}

std::unique_ptr<std::vector<float>> PrecomputeValueToBoundedFloat(
    const uint16_t unknown_value, const float unknown_result,
    const float lower_bound, const float upper_bound)
{
    auto result = std::make_unique<std::vector<float>>();
    // Repeat two times, so that both values with and without the update marker
    // can be converted to a probability.
    // 重复2遍
    constexpr int kRepetitionCount = 2;
    result->reserve(kRepetitionCount * kValueCount);
    for (int repeat = 0; repeat != kRepetitionCount; ++repeat)
    {
        for (int value = 0; value != kValueCount; ++value)
        {
            result->push_back(SlowValueToBoundedFloat(
                value, unknown_value, unknown_result, lower_bound, upper_bound));
        }
    }
    return result;
}

std::unique_ptr<std::vector<float>> PrecomputeValueToCorrespondenceCost()
{
    return PrecomputeValueToBoundedFloat(
        0, 0.9,    // 0,   0.9
        0.1, 0.9); // 0.1, 0.9
}

const std::vector<float> *const kValueToCorrespondenceCost =
    PrecomputeValueToCorrespondenceCost().release();

float Clamp(const float value, const float min, const float max)
{
    if (value > max)
    {
        return max;
    }
    if (value < min)
    {
        return min;
    }
    return value;
}

uint16_t BoundedFloatToValue(const float float_value,
                             const float lower_bound,
                             const float upper_bound)
{
    const int value =
        std::lround(
            (Clamp(float_value, lower_bound, upper_bound) - lower_bound) *
            (32766.f / (upper_bound - lower_bound))) +
        1;
    return value;
}

uint16_t CorrespondenceCostToValue(const float correspondence_cost)
{
    return BoundedFloatToValue(correspondence_cost, 0.1, 0.9);
}

uint16_t getNextNum(float n)
{
    return CorrespondenceCostToValue(ProbabilityToCorrespondenceCost(ProbabilityFromOdds(n)));
}

std::vector<uint16_t> ComputeLookupTableToApplyCorrespondenceCostOdds(
    float odds)
{
    std::vector<uint16_t> result;
    result.reserve(32768); // 32768 = 2^15

    // 当前cell是unknown情况下直接把odds转成value存进来
    result.push_back(CorrespondenceCostToValue(ProbabilityToCorrespondenceCost(
        ProbabilityFromOdds(odds)))); // 加上kUpdateMarker作为一个标志, 代表这个栅格已经被更新了
    // 计算更新时 从1到32768的所有可能的 更新后的结果
    for (int cell = 1; cell != kValueCount; ++cell)
    {
        result.push_back(
            CorrespondenceCostToValue(
                ProbabilityToCorrespondenceCost(ProbabilityFromOdds(
                    odds * Odds(CorrespondenceCostToProbability(
                               (*kValueToCorrespondenceCost)[cell]))))));
    }
    return result;
}

int main()
{
    // 建立hit table
    std::vector<uint16_t> hit_table = ComputeLookupTableToApplyCorrespondenceCostOdds(0.55);
    cout << "please input 1 to start look up the hit table" << endl;

    // 输入index，输出value。index是当前概率，value是下一个概率值
    int index;
    while (cin >> index)
    {
        uint16_t value = hit_table[index];
        cout << "value=" << value << " => odds=" << SlowValueToBoundedFloat(value, 0, 0.1, 0.1, 0.9) << endl;
    }
}