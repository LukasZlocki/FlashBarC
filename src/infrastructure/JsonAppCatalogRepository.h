#pragma once

#include "../repositories/AppCatalogRepository.h"

#include <QString>

class JsonAppCatalogRepository final : public AppCatalogRepository
{
public:
    explicit JsonAppCatalogRepository(QString filePath);

    std::vector<domain::AppEntry> loadApps() const override;
    bool saveApp(const domain::AppEntry &appEntry) override;

private:
    QString filePath_;
};