﻿#pragma once

#include "InvertedIndex.h"
#include <thread>
#include <mutex>


void recordWords (std::vector<std::vector<std::vector<Entry>>> &VVVEntry, std::vector<std::vector<std::string>> VVString, std::vector<std::vector<Entry>> VEntry, int i, InvertedIndex _index)
{
    std::mutex mtx;
    for (int j = 0; j < VVString[i].size(); ++j)
    {
        mtx.lock();
        auto r = _index.GetWordCount(VVString[i][j]);
        VEntry.push_back(r);
        mtx.unlock();
    }
    mtx.lock();
    VVVEntry.push_back(VEntry);
    VEntry.clear();
    mtx.unlock();
}

struct RelativeIndex{
    size_t doc_id;
    float rank;
    bool operator ==(const RelativeIndex& other) const {
        return (doc_id == other.doc_id && rank == other.rank);
    }
};
class SearchServer {
public:

/**
* @param idx в конструктор класса передаётся ссылка на класс
InvertedIndex,
* чтобы SearchServer мог узнать частоту слов встречаемых в
запросе
*/
    SearchServer(InvertedIndex &idx) : _index(idx) {};

/**
* Метод обработки поисковых запросов
* @param queries_input поисковые запросы взятые из файла
requests.json
* @return возвращает отсортированный список релевантных ответов для
заданных запросов
*/
    std::vector<std::vector<RelativeIndex>> search(const std::vector<std::string> &queries_input) {

        std::vector<std::vector<std::string>> VVString;
        std::vector<std::string> VString;
        std::string resultWord;

        for (int j = 0; j < queries_input.size(); ++j) {
            for (int i = 0; i <= queries_input[j].size(); ++i) {
                if (queries_input[j][i] == ' ' || i == queries_input[j].size()) {
                    VString.push_back(resultWord);
                    resultWord = "";
                } else resultWord += queries_input[j][i];
            }
            VVString.push_back(VString);
            VString.clear();
        }

        std::vector<std::vector<std::vector<Entry>>> VVVEntry;
        std::vector<std::vector<Entry>> VEntry;
        std::vector<std::thread> threads;

        for (int i = 0; i < VVString.size(); ++i) {
            threads.emplace_back(recordWords, std::ref(VVVEntry), VVString, VEntry, i, _index);
            threads[i].join();
        }

        int sum = 0;
        int max = 0;
        std::vector<std::vector<Entry>> VVResultEntry;
        std::vector<Entry> VResultEntry;
        Entry resultEntry;

        for (int i = 0; i < VVVEntry.size(); ++i) {
            for (int m = 0; m < converterJson.GetTextDocuments().size(); ++m) {
                for (int j = 0; j < VVVEntry[i].size(); ++j) {
                    for (int k = 0; k < VVVEntry[i][j].size(); ++k) {
                        if (VVVEntry[i][j][k].doc_id == m) {
                            sum += VVVEntry[i][j][k].count;
                        }
                    }
                }
                if (sum != 0) {
                    if (sum > max) {
                        max = sum;
                    }
                    resultEntry.doc_id = m;
                    resultEntry.count = sum;
                    VResultEntry.push_back(resultEntry);
                    sum = 0;
                }
            }
            VVResultEntry.push_back(VResultEntry);
            VResultEntry.clear();
        }

        std::vector<std::vector<RelativeIndex>> VVRelativeIndex;
        std::vector<RelativeIndex> VRelativeIndex;
        RelativeIndex relativeIndex;

        for (int i = 0; i < VVResultEntry.size(); ++i) {
            for (int j = 0; j < VVResultEntry[i].size(); ++j) {
                relativeIndex.doc_id = VVResultEntry[i][j].doc_id;
                relativeIndex.rank = float(VVResultEntry[i][j].count) / float(max);
                VRelativeIndex.push_back(relativeIndex);
            }
            VVRelativeIndex.push_back(VRelativeIndex);
            VRelativeIndex.clear();
        }

        return VVRelativeIndex;
    };


    std::vector<std::vector<std::pair<int, float>>> completionAnswers() {
        auto VVRelativeIndex = search(converterJson.GetRequests());
        std::vector<std::vector<std::pair<int, float>>> VVAnswers;
        std::vector<std::pair<int, float>> VAnswers;
        std::pair<int, float> answers;

        for (int i = 0; i < VVRelativeIndex.size(); ++i) {
            for (int j = 0; j < VVRelativeIndex[i].size(); ++j) {
                answers = {VVRelativeIndex[i][j].doc_id, VVRelativeIndex[i][j].rank};
                VAnswers.push_back(answers);
            }
            if (VVRelativeIndex[i].empty()) {
                answers = {0, 0};
                VAnswers.push_back(answers);
            }
            VVAnswers.push_back(VAnswers);
            VAnswers.clear();
        }

        for (int i = 0; i < VVAnswers.size(); ++i) {
            for (int k = 0; k < VVAnswers[i].size(); ++k) {
                for (int j = 1; j < VVAnswers[i].size(); ++j) {
                    if (VVAnswers[i][j].second > VVAnswers[i][j - 1].second) {
                        std::swap(VVAnswers[i][j], VVAnswers[i][j - 1]);
                    }
                }
            }
        }

        return VVAnswers;
    }

private:
    ConverterJSON converterJson;
    InvertedIndex _index;
};