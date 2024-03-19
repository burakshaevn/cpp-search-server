#pragma once

#include <iostream>

using namespace std::string_literals;

enum class DocumentStatus {
    ACTUAL, // актуальный 
    IRRELEVANT, // устаревший 
    BANNED, // отклонённый 
    REMOVED // удалённый 
};

struct Document {
    Document() : id(0), relevance(0.0), rating(0), status_(DocumentStatus::ACTUAL)
    {}
    Document(int _id, double _relevance, int _rating)
        : id(_id), relevance(_relevance), rating(_rating), status_(DocumentStatus::ACTUAL)
    {}
    Document(int _id, double _relevance, int _rating, DocumentStatus _status) 
        : id(_id), relevance(_relevance), rating(_rating), status_(_status)
    {}
    int id;
    double relevance;
    int rating;
    DocumentStatus status_;
};

void PrintDocument(const Document& document);

std::ostream& operator<<(std::ostream& out, const Document& document);