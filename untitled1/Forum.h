#ifndef FORUM_H
#define FORUM_H
#include "ForumInfo.h"
#include "Trash.h"
#include <BdWrapper.h>
#include <HandleTemplates.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSqlQuery>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <ThreadInfo.h>
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
        HandleRequestBase hR;

        hR.handlePostParams(request);
        QString conName = BdWrapper::getConnection();
        bool test = QSqlDatabase::database(conName).transaction();

        QSqlQuery query(QSqlDatabase::database(conName));
        query.prepare("INSERT INTO Forums (name, short_name, user) VALUES (:name, :short_name, :user);");
        query.bindValue(":name", hR.objectRequest["name"].toString());
        query.bindValue(":short_name", hR.objectRequest["short_name"].toString());
        query.bindValue(":user", hR.objectRequest["user"].toString());
        bool ok = query.exec();
        bool o2 = QSqlDatabase::database(conName).commit();


        hR.handleResponse();
        //проверка на код и всё такое тут должны быть
        hR.objectResponce["code"] = ok ? 0 : 5;
        bool isForumExist = true;
        hR.objectResponce["response"] = ForumInfo::getForumCreateInfo(hR.objectRequest["short_name"].toString(), isForumExist);

        hR.prepareOutput();

        response.setStatus(200);

        response.out() << hR.output;
        BdWrapper::closeConnection(conName);
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
        HandleRequestBase hR;
        QString related;
        related = related.fromStdString(request.getParameter("related") ? *request.getParameter("related") : " ");
        QString short_name;
        short_name = short_name.fromStdString(request.getParameter("forum") ? *request.getParameter("forum") : " ");

        // QString user;

        bool isForumExist = false;

        hR.handleResponse();
        if (related != " ") {
            hR.responseContent = ForumInfo::getFullForumInfo(short_name, isForumExist);
        } else {
            hR.responseContent = ForumInfo::getForumCreateInfo(short_name, isForumExist);
        }

        hR.objectResponce["response"] = hR.responseContent;

        hR.objectResponce["code"] = isForumExist ? 0 : 1;

        hR.prepareOutput();
        response.setStatus(200);

        response.out() << hR.output;
    };
};

class ForumListPosts : public Wt::WResource {
public:
    virtual ~ForumListPosts()
    {
        beingDeleted();
    };

protected:
    virtual void handleRequest(const Wt::Http::Request& request, Wt::Http::Response& response)
    {
         HandleRequestBase hR;
        QString forum;
        forum = forum.fromStdString(request.getParameter("forum") ? *request.getParameter("forum") : "");

        QString order;
        order = order.fromStdString(request.getParameter("order") ? *request.getParameter("order") : "");
        //0 - magic constant for empty parametr
        QString since_id;
        since_id = since_id.fromStdString(request.getParameter("since") ? *request.getParameter("since") : "");
        QString limit;
        limit = limit.fromStdString(request.getParameter("limit") ? *request.getParameter("limit") : "");

        QString relatedArray[3]; //0-user, 1-forum,3 -thread
        relatedArray[0] = "";
        relatedArray[1] = "";
        relatedArray[2] = "";

        auto temp = request.getParameterValues("related");

        for (auto i = temp.begin(); i != temp.end(); i++) {
            if ((*i) == "user")
                relatedArray[0] = "user";
            else if ((*i) == "forum")
                relatedArray[1] = "forum";
            else
                relatedArray[2] = "thread";
        }

        QString str_since = " ";
        QString str_limit = " ";
        QString str_order = " ";
        QString quote = "\"";

        if (since_id != "")
            str_since = " AND p.date >= " + quote + since_id + quote;

        if (limit != "")
            str_limit = " LIMIT " + limit;
        if (order == "asc")
            str_order = " ORDER BY p.date asc";
        else
            str_order = " ORDER BY p.date  desc";

        // QString user;

        hR.handleResponse();
        QJsonArray arrayOfPosts;
        QString expression = "SELECT id, user, message,forum,thread_id, parent, date,likes, dislikes,isApproved,isHighlighted,isEdited,isSpam,isDeleted FROM Posts p WHERE p.forum=" + quote + forum + quote + str_since + str_order + str_limit + ";";
        QString conName = BdWrapper::getConnection();
        QSqlQuery query(QSqlDatabase::database(conName));
        bool ok = query.exec(expression);

        while (query.next()) {
            QString strGoodReply = Source::getPostTemplate();
            QJsonDocument jsonResponse = QJsonDocument::fromJson(strGoodReply.toUtf8());
            QJsonObject jsonArray = jsonResponse.object();

            //   QJsonObject jsonArray;
            jsonArray["id"] = query.value(0).toInt();
            jsonArray["user"] = query.value(1).toString();
            jsonArray["message"] = query.value(2).toString();
            jsonArray["forum"] = query.value(3).toString();
            jsonArray["thread"] = query.value(4).toInt();

            if (!query.value(5).isNull())
                jsonArray["parent"] = query.value(5).toInt();
            else
                jsonArray["parent"] = QJsonValue::Null;

            jsonArray["date"] = query.value(6).toDateTime().toString("yyyy-MM-dd hh:mm:ss");

            jsonArray["likes"] = query.value(7).toInt();
            jsonArray["dislikes"] = query.value(8).toInt();
            jsonArray["isApproved"] = query.value(9).toBool();
            jsonArray["isHighlighted"] = query.value(10).toBool();
            jsonArray["isEdited"] = query.value(11).toBool();
            jsonArray["isSpam"] = query.value(12).toBool();
            jsonArray["isDeleted"] = query.value(13).toBool();
            jsonArray["points"] = query.value(7).toInt() - query.value(8).toInt();

            bool isUserExist = true; // костыль
            if (relatedArray[0] != "") {
                jsonArray["user"] = UserInfo::getFullUserInfo(jsonArray["user"].toString(), isUserExist);
            }
            if (relatedArray[1] != "") { //TODO
                jsonArray["forum"] = ForumInfo::getForumCreateInfo(forum, isUserExist);
            }
            if (relatedArray[2] != "") { //TODO
                jsonArray["thread"] = ThreadInfo::getFullThreadInfo(jsonArray["thread"].toInt(), isUserExist);
            }
            arrayOfPosts << jsonArray;
        }
        std::cout << query.lastQuery().toStdString() << "QUERY";
        hR.objectResponce["response"] = arrayOfPosts;

        hR.objectResponce["code"] = ok ? 0 : 1;

        hR.prepareOutput();
        response.setStatus(200);

        response.out() << hR.output;
        BdWrapper::closeConnection(conName);

    }
};

class ForumListThreads : public Wt::WResource{
public:
    virtual ~ForumListThreads()
    {
        beingDeleted();
    };

protected:
    virtual void handleRequest(const Wt::Http::Request& request, Wt::Http::Response& response)
    {
        HandleRequestList hR;
        QString forum;
        forum = forum.fromStdString(request.getParameter("forum") ? *request.getParameter("forum") : "");

        QString order;
        order = order.fromStdString(request.getParameter("order") ? *request.getParameter("order") : "");
        //0 - magic constant for empty parametr
        QString since_id;
        since_id = since_id.fromStdString(request.getParameter("since") ? *request.getParameter("since") : "");
        QString limit;
        limit = limit.fromStdString(request.getParameter("limit") ? *request.getParameter("limit") : "");

        QString relatedArray[2]; //0-user, 1-forum
        relatedArray[0] = "";
        relatedArray[1] = "";

        auto temp = request.getParameterValues("related");

        for (auto i = temp.begin(); i != temp.end(); i++) {
            if ((*i) == "user")
                relatedArray[0] = "user";
            else if ((*i) == "forum")
                relatedArray[1] = "forum";
        }

        QString str_since = " ";
        QString str_limit = " ";
        QString str_order = " ";
        QString quote = "\"";

        if (since_id != "")
            str_since = " AND p.date >= " + quote + since_id + quote;

        if (limit != "")
            str_limit = " LIMIT " + limit;
        if (order == "asc")
            str_order = " ORDER BY p.date asc";
        else
            str_order = " ORDER BY p.date  desc";

        // QString user;

        hR.handleResponse();
        QJsonArray arrayOfPosts;
        QString expression = "SELECT id,forum,user,title,slug,message,date,likes,dislikes,isClosed,isDeleted FROM Threads p WHERE p.forum=" + quote + forum + quote + str_since + str_order + str_limit + ";";
        QString conName = BdWrapper::getConnection();
        QSqlQuery query(QSqlDatabase::database(conName));
        bool ok = query.exec(expression);

        while (query.next()) {
            QString strGoodReply = Source::getFullThreadTemplate();
            QJsonDocument jsonResponse = QJsonDocument::fromJson(strGoodReply.toUtf8());
            //  QJsonObject hR.objectResponce = jsonResponse.object();
            QJsonObject jsonArray = jsonResponse.object();

            //   QJsonObject jsonArray;
            jsonArray["id"] = query.value(0).toInt();
            jsonArray["forum"] = query.value(1).toString();
            jsonArray["user"] = query.value(2).toString();
            jsonArray["title"] = query.value(3).toString();
            jsonArray["slug"] = query.value(4).toString();
            jsonArray["message"] = query.value(5).toString();

            jsonArray["date"] = query.value(6).toDateTime().toString("yyyy-MM-dd hh:mm:ss");

            jsonArray["likes"] = query.value(7).toInt();
            jsonArray["dislikes"] = query.value(8).toInt();
            jsonArray["isClosed"] = query.value(9).toBool();
            jsonArray["isDeleted"] = query.value(10).toBool();
            jsonArray["points"] = query.value(7).toInt() - query.value(8).toInt();
            jsonArray["posts"] = PostInfo::countPosts(jsonArray["id"].toInt());

            bool isUserExist = true; // костыль
            if (relatedArray[0] != "") {
                jsonArray["user"] = UserInfo::getFullUserInfo(jsonArray["user"].toString(), isUserExist);
            }
            if (relatedArray[1] != "") {
                jsonArray["forum"] = ForumInfo::getForumCreateInfo(forum, isUserExist);
            }

            arrayOfPosts << jsonArray;
        }
        std::cout << query.lastQuery().toStdString() << "QUERY";
        hR.objectResponce["response"] = arrayOfPosts;

        hR.objectResponce["code"] = ok ? 0 : 1;

        hR.prepareOutput();
        response.setStatus(200);

        response.out() << hR.output;
        BdWrapper::closeConnection(conName);

    }
};

class ForumListUsers : public Wt::WResource{
public:
    virtual ~ForumListUsers()
    {
        beingDeleted();
    }

protected:
    virtual void handleRequest(const Wt::Http::Request& request, Wt::Http::Response& response)
    {
        HandleRequestList  hR;
        QString forum;
        forum = forum.fromStdString(request.getParameter("forum") ? *request.getParameter("forum") : " ");

        QString order;
        order = order.fromStdString(request.getParameter("order") ? *request.getParameter("order") : " ");
        //0 - magic constant for empty parametr
        QString since_id;
        since_id = forum.fromStdString(request.getParameter("since_id") ? *request.getParameter("since_id") : " ");
        QString limit;
        limit = forum.fromStdString(request.getParameter("limit") ? *request.getParameter("limit") : " ");

        QString str_since;
        QString str_limit;
        QString str_order;
        QString quote = "\"";

        if (since_id != " ")
            str_since = " AND u.id >= " + quote + since_id + quote;

        if (limit != " ")
            str_limit = " LIMIT " + limit;
        if (order == "asc")
            str_order = " ORDER BY u.name asc ";
        else
            str_order = " ORDER BY u.name desc ";
        QString conName = BdWrapper::getConnection();
        QSqlQuery query(QSqlDatabase::database(conName));
        QString expression;
        expression = "SELECT distinct u.id, u.email,u.username, u.about, u.name, u.isAnonymous FROM Users u JOIN Posts p on u.email = p.user WHERE p.forum=" + quote + forum + quote + str_since + str_order + str_limit + ";";
        //str_since + str_order + str_limit + ";";

        bool ok = query.exec(expression);

        hR.handleResponse();
        QJsonArray arrayOfThreads;
        bool isThreadExist = true; // заглушка

        if (ok) {
            while (query.next()) {
                QString strGoodReply = Source::getUserTemplate();
                QJsonDocument jsonResponse = QJsonDocument::fromJson(strGoodReply.toUtf8());
                //  QJsonObject hR.objectResponce = jsonResponse.object();
                QJsonObject jsonArray = jsonResponse.object();
                jsonArray["id"] = query.value(0).toInt();
                jsonArray["email"] = query.value(1).toString();
                if (query.value(2).isNull())

                    jsonArray["username"] = QJsonValue::Null;
                else
                    jsonArray["username"] = query.value(2).toString();

                jsonArray["about"] = query.value(3).toString();
                if (jsonArray["about"] == "")
                    jsonArray["about"] = QJsonValue::Null;

                jsonArray["name"] = query.value(4).toString();
                if (jsonArray["name"] == "")
                    jsonArray["name"] = QJsonValue::Null;

                jsonArray["isAnonymous"] = query.value(5).toBool();

                UserInsideInfo userInsideInfo;

                QJsonArray followers = userInsideInfo.getFollowers(jsonArray["email"].toString());
                QJsonArray followee = userInsideInfo.getFollowee(jsonArray["email"].toString());
                //   QJsonObject jsonArray;
                jsonArray["following"] = followee;
                jsonArray["followers"] = followers;

                QJsonArray subscriptions = userInsideInfo.getSubscriptions(jsonArray["email"].toString());
                jsonArray["subscriptions"] = subscriptions;
                std::cout << query.lastQuery().toStdString() << "allah";

                arrayOfThreads << jsonArray;
            }
        }
        hR.objectResponce["response"] = arrayOfThreads;
        std::cout << query.lastQuery().toStdString() << "USERLIST";
        hR.prepareOutput();

        response.setStatus(200);

        response.out() << hR.output;
        BdWrapper::closeConnection(conName);

    }
};
#endif // FORUM_H
