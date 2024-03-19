#pragma once

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include<numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include "document.h"
#include "string_processing.h"

using namespace std::string_literals;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

const double EPSILON = 1e-6;

class SearchServer {
public:

    SearchServer() = default;

    explicit SearchServer(const std::string& container) 
        : SearchServer(std::set<std::string>{container})
    {}

    template<typename ContainerType>
    explicit  SearchServer(const ContainerType& container) {
        std::set<std::string> set_stop_words;
        for (const auto& stop_word : container) {
            if (!stop_word.empty()) {
                if (!IsValidWord(stop_word)) {
                    throw std::invalid_argument("Строка содержит недопустимые символы."s);
                }
                else {
                    set_stop_words.insert(stop_word);
                }
            }
            else continue;
        }
        for (const auto& stop_word : set_stop_words) {
            SetStopWords(stop_word);
        }
    }

    int GetDocumentId(int index) const;

    int GetDocumentCount() const;

    void SetStopWords(const std::string& text);

    void AddDocument(int document_id, const std::string& raw_query, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words, document_predicate); // returns: id, relevance, rating_, status_ 

        std::sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (lhs.relevance != rhs.relevance) {
                return lhs.relevance > rhs.relevance;
            }
            else if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            }
            else return false;
            });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status_) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

private:
    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    struct DocumentData {
        int rating_;
        DocumentStatus status_;
    };

    std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> words_freqs_; // returns: <word, std::map<id_document, relevance>>  
    std::map<int, DocumentData> documents_; // returns: <document_id, <rating_, status_>>   
    std::vector<int> documents_ids_;

    static bool IsValidWord(const std::string& word);

    static bool IsValidMinusWord(const std::string& word);

    template <typename T>
    std::vector<Document> FindAllDocuments(const Query& query_words, T status) const {
        std::map<int, double> document_to_relevance; // <document_id, relevance> 
        for (const std::string& plus_word : query_words.plus_words) {
            if (words_freqs_.count(plus_word)) {
                for (auto& [document_id, tf] : words_freqs_.at(plus_word)) {
                    if (status(document_id, documents_.at(document_id).status_, documents_.at(document_id).rating_)) {
                        document_to_relevance[document_id] += ComputeIDF(plus_word) * tf;
                    }
                }
            }
        }
        for (const std::string& minus_word : query_words.minus_words) {
            if (words_freqs_.count(minus_word)) {
                for (const auto& [document_id, relevance] : words_freqs_.at(minus_word)) {
                    document_to_relevance.erase(document_id);
                }
            }
        }
        std::vector<Document> matched_documents;
        for (auto& [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating_, documents_.at(document_id).status_ });
        }
        return matched_documents;
    }

    static int ComputeAverageRating(const std::vector<int>& ratings_);

    double ComputeIDF(const std::string& plus_word) const;

    bool IsStopWord(const std::string& word) const;

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    Query ParseQuery(const std::string& text) const;
};