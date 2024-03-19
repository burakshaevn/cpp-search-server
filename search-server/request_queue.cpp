#include "request_queue.h"

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    const auto result = search_server_.FindTopDocuments(raw_query, document_predicate);
    RecordTheResult(result.size());
    return result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    const auto result = search_server_.FindTopDocuments(raw_query, status);
    RecordTheResult(result.size());
    return result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    const auto result = search_server_.FindTopDocuments(raw_query);
    RecordTheResult(result.size());
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    return no_result_requests;
}

void RequestQueue::RecordTheResult(size_t size) {
    ++current_time;

    // Удаление всех результатов поиска, которые устарели
    while (!requests_.empty() && min_in_day_ <= current_time - requests_.front().timestamp) {
        if (requests_.front().results == 0) {
            --no_result_requests;
        }
        requests_.pop_front();
    }

    // Сохранение нового результата поиска
    requests_.push_back({ current_time, size });
    if (size == 0) {
        ++no_result_requests;
    }
}