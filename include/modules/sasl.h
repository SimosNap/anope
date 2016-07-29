/*
 *
 * (C) 2014-2016 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 */

namespace SASL
{
	struct Message
	{
		Anope::string source;
		Anope::string target;
		Anope::string type;
		Anope::string data;
		Anope::string ext;
	};

	class Mechanism;
	struct Session;

	class Service : public ::Service
	{
	 public:
		static constexpr const char *NAME = "sasl";
		
		Service(Module *o) : ::Service(o, NAME) { }

		virtual void ProcessMessage(const Message &) anope_abstract;

		virtual Anope::string GetAgent() anope_abstract;

		virtual Session* GetSession(const Anope::string &uid) anope_abstract;

		virtual void SendMessage(SASL::Session *session, const Anope::string &type, const Anope::string &data) anope_abstract;

		virtual void Succeed(Session *, NickServ::Account *) anope_abstract;
		virtual void Fail(Session *) anope_abstract;
		virtual void SendMechs(Session *) anope_abstract;
		virtual void DeleteSessions(Mechanism *, bool = false) anope_abstract;
		virtual void RemoveSession(Session *) anope_abstract;
	};

	class Session
	{
		SASL::Service *service;
		
	 public:
		time_t created;
		Anope::string uid;
		Reference<Mechanism> mech;

		Session(SASL::Service *s, Mechanism *m, const Anope::string &u) : service(s), created(Anope::CurTime), uid(u), mech(m) { }
		
		virtual ~Session()
		{
			service->RemoveSession(this);
		}
	};

	/* PLAIN, EXTERNAL, etc */
	class Mechanism : public ::Service
	{
		SASL::Service *service;
		
	 public:
		static constexpr const char *NAME = "sasl/mechanism";
		
		Mechanism(SASL::Service *s, Module *o, const Anope::string &sname) : Service(o, NAME, sname), service(s) { }
		
		SASL::Service *GetService() const { return service; }

		virtual Session* CreateSession(const Anope::string &uid) { return new Session(service, this, uid); }

		virtual void ProcessMessage(Session *session, const Message &) anope_abstract;

		virtual ~Mechanism()
		{
			service->DeleteSessions(this, true);
		}
	};

	class IdentifyRequestListener : public NickServ::IdentifyRequestListener
	{
		SASL::Service *service = nullptr;
		Anope::string uid;

	 public:
		IdentifyRequestListener(SASL::Service *s, const Anope::string &id) : service(s), uid(id) { }

		void OnSuccess(NickServ::IdentifyRequest *req) override;

		void OnFail(NickServ::IdentifyRequest *req) override;
	};
}
