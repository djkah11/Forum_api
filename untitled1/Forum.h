#ifndef FORUM_H
#define FORUM_H
#include "Trash.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSqlQuery>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <Wt/Http/Response>
#include <Wt/Json/Object>
#include <Wt/Json/Parser>
#include <Wt/Json/Serializer>
#include <Wt/WResource>
#include <Wt/WServer>
#include <iostream>
#include <istream>

class ForumCreate : public Wt::WResource {
public:
    virtual ~ForumCreate()
    {
        beingDeleted();
    };

protected:
    virtual void handleRequest(const Wt::Http::Request& request, Wt::Http::Response& response)
    {
        QString params = LineAnalyze::getRequestBody(request);
        //  auto temp4 = Wt::Json::parse(request);
        /*auto temp5 = request.queryString();
        auto temp1 = request.contentType();
        auto temp2 = request.contentLength();
        auto temp6 = request.continuation();

        auto temp3 = request.getParameter("name");
        std::string temp7;
       // auto temp8 = request.in();
        while (std::getline(request.in(),temp7));
          params.fromStdString(temp7);
         //иногда падает с ошибкой почему-то

    */

        QJsonDocument jsonRequest = QJsonDocument::fromJson(params.toUtf8());

        QJsonObject jsonObject2 = jsonRequest.object();
        QString name = jsonObject2["name"].toString();
        QString short_name = jsonObject2["short_name"].toString();
        QString user = jsonObject2["user"].toString();

        QString strReply = "{ "
                           "  \"code\": 0, "
                           "  \"response\":{\"id\": 1,\"name\": \"Forum With Sufficiently Large Name\",\"short_name\": \"forumwithsufficientlylargename\",\"user\": \"richard.nixon@example.com\"} "
                           "}";
        QSqlQuery query(QSqlDatabase::database("apidb1"));
        query.exec("SHOW TABLES;");
        bool ok2 = query.exec();
        query.clear();
        query.prepare("INSERT INTO Forums (name, short_name, user) VALUES (:name, :short_name, :user);");
        query.bindValue(":name", name);
        query.bindValue(":short_name", short_name);
        query.bindValue(":user", user);
        bool ok = query.exec();

        QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
        QJsonObject jsonObject = jsonResponse.object();

        QJsonObject jsonArray = jsonObject["response"].toObject();
        if (ok) {
            jsonArray["name"] = name;
            jsonArray["short_name"] = short_name;
            jsonArray["user"] = user;
            jsonObject["response"] = jsonArray;
        } else {
            jsonArray["name"] = query.value(0).toString();
            jsonArray["short_name"] = query.value(1).toString();
            jsonArray["user"] = query.value(2).toString();
            jsonObject["response"] = jsonArray;
        }
        QJsonDocument doc(jsonObject);
        QByteArray data = doc.toJson();

        response.setStatus(200);

        response.out() << data.toStdString();
    }
};

class ForumDetails : public Wt::WResource {
public:
    virtual ~ForumDetails()
    {
        beingDeleted();
    };

protected:
    virtual void handleRequest(const Wt::Http::Request& request, Wt::Http::Response& response)
    {
        auto detatils = request.getParameter("details");
    };
};

#endif // FORUM_H
