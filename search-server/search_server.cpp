#include "search_server.h"
#include "string_processing.h"

int SearchServer::GetDocumentId(int index) const {
    try {
        return documents_ids_.at(index);
    }
    catch (const std::exception&) {
        throw std::out_of_range("Значение id выходит за пределы [0, " + std::to_string(GetDocumentCount()) + ").");
    }
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

void SearchServer::SetStopWords(const std::string& text) {
    for (const std::string& word : SplitIntoWords(text)) {
        stop_words_.insert(word);
    }
}

void SearchServer::AddDocument(int document_id, const std::string& raw_query, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("Получено отрицательное значение id."s);
    }
    if (std::count(documents_ids_.begin(), documents_ids_.end(), document_id) > 0) {
        throw std::invalid_argument("Попытка добавить документ с существующим id."s);
    }
    const std::vector<std::string>& words = SplitIntoWordsNoStop(raw_query);
    const double temp = 1. / static_cast<double>(words.size());
    // TF - частота слова в конкретном документе
    for (const std::string& word : words) {
        words_freqs_[word][document_id] += temp;
    }
    documents_ids_.push_back(document_id);
    documents_[document_id].rating_ = ComputeAverageRating(ratings);
    documents_[document_id].status_ = status;
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status_) const {
    return  FindTopDocuments(raw_query, [&status_](const int document_id, const DocumentStatus t_status, const int rating_) {
        return status_ == t_status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return  FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    const Query query = ParseQuery(raw_query);
    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) { // проходимся по плюс-словам 
        if (words_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (words_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return std::tie(matched_words, documents_.at(document_id).status_);
}

 bool SearchServer::IsValidWord(const std::string& word) { // Проверка на наличие спецсимволов
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

bool SearchServer::IsValidMinusWord(const std::string& word) { // Проверка на ненужный минус
    if ((word[0] == '-' and word[1] == '-') or word.empty()) {
        return false;
    }
    if (word == "-"s) {
        return false;
    }
    return true;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings_) {
    if (ratings_.empty()) {
        return 0;
    }
    else {
        return std::accumulate(ratings_.begin(), ratings_.end(), 0) / static_cast<int>(ratings_.size());
    }
}

double SearchServer::ComputeIDF(const std::string& plus_word) const {
    return log(static_cast<double>(documents_.size()) / words_freqs_.at(plus_word).size());
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Строка содержит недопустимые символы."s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const { // поиск по запросу 
    for (const auto& token : SplitIntoWords(text)) {
        if (!IsValidWord(token)) {
            throw std::invalid_argument("Присутствуют недопустимые символы с кодами [0, 31]."s);
        }
        if (!IsValidMinusWord(token)) {
            throw std::invalid_argument("Строка имеет некорректное расположение/количество минуса.");
        }
    }
    Query query_words;
    for (const std::string& word : SplitIntoWordsNoStop(text)) {
        if (word.at(0) != '-') {
            query_words.plus_words.insert(word);
        }
        else {
            query_words.minus_words.insert(word.substr(1));
        }
    }
    return query_words;
}