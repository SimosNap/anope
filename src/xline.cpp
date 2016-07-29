/* XLine functions.
 *
 * (C) 2003-2014 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 *
 */

#include "services.h"
#include "modules.h"
#include "xline.h"
#include "users.h"
#include "sockets.h"
#include "config.h"
#include "commands.h"
#include "servers.h"

/* List of XLine managers we check users against in XLineManager::CheckAll */
std::vector<XLineManager *> XLineManager::XLineManagers;

void XLine::Recache()
{
	delete regex;
	regex = nullptr;

	delete c;
	c = nullptr;

	Anope::string mask = GetMask();
	if (mask.length() >= 2 && mask[0] == '/' && mask[mask.length() - 1] == '/' && Config->regex_flags)
	{
		Anope::string stripped_mask = mask.substr(1, mask.length() - 2);

		try
		{
			this->regex = new std::regex(stripped_mask.str(), Config->regex_flags);
		}
		catch (const std::regex_error &ex)
		{
			Log(LOG_DEBUG) << ex.what();
		}
	}

	size_t nick_t = mask.find('!');
	if (nick_t != Anope::string::npos)
		nick = mask.substr(0, nick_t);

	size_t user_t = mask.find('!'), host_t = mask.find('@');
	if (host_t != Anope::string::npos)
	{
		if (user_t != Anope::string::npos && host_t > user_t)
			user = mask.substr(user_t + 1, host_t - user_t - 1);
		else
			user = mask.substr(0, host_t);
	}

	size_t real_t = mask.find('#');
	if (host_t != Anope::string::npos)
	{
		if (real_t != Anope::string::npos && real_t > host_t)
			host = mask.substr(host_t + 1, real_t - host_t - 1);
		else
			host = mask.substr(host_t + 1);
	}
	else
	{
		if (real_t != Anope::string::npos)
			host = mask.substr(0, real_t);
		else
			host = mask;
	}

	if (real_t != Anope::string::npos)
		real = mask.substr(real_t + 1);

	if (host.find('/') != Anope::string::npos)
	{
		c = new cidr(host);
		if (!c->valid())
		{
			delete c;
			c = NULL;
		}
	}
}

XLine::~XLine()
{
	delete regex;
	delete c;
}

void XLine::SetType(const Anope::string &t)
{
	Set(&XLineType::type, t);
}

Anope::string XLine::GetType()
{
	return Get(&XLineType::type);
}

void XLine::SetMask(const Anope::string &m)
{
	Set(&XLineType::mask, m);
}

Anope::string XLine::GetMask()
{
	return Get<Anope::string>(&XLineType::mask);
}

void XLine::SetBy(const Anope::string &b)
{
	Set(&XLineType::by, b);
}

Anope::string XLine::GetBy()
{
	return Get(&XLineType::by);
}

void XLine::SetReason(const Anope::string &r)
{
	Set(&XLineType::reason, r);
}

Anope::string XLine::GetReason()
{
	return Get(&XLineType::reason);
}

void XLine::SetID(const Anope::string &id)
{
	Set(&XLineType::id, id);
}

Anope::string XLine::GetID()
{
	return Get(&XLineType::id);
}

void XLine::SetCreated(const time_t &t)
{
	Set(&XLineType::created, t);
}

time_t XLine::GetCreated()
{
	return Get(&XLineType::created);
}

void XLine::SetExpires(const time_t &e)
{
	Set(&XLineType::expires, e);
}

time_t XLine::GetExpires()
{
	return Get(&XLineType::created);
}

const Anope::string &XLine::GetNick() const
{
	return nick;
}

const Anope::string &XLine::GetUser() const
{
	return user;
}

const Anope::string &XLine::GetHost() const
{
	return host;
}

const Anope::string &XLine::GetReal() const
{
	return real;
}

Anope::string XLine::GetReasonWithID()
{
	Anope::string r = this->GetReason();
	if (!this->GetID().empty())
		r += " (ID: " + this->GetID() + ")";
	return r;
}

bool XLine::HasNickOrReal() const
{
	return !this->GetNick().empty() || !this->GetReal().empty();
}

bool XLine::IsRegex()
{
	Anope::string mask = GetMask();
	return mask.length() > 2 && mask[0] == '/' && mask[mask.length() - 1] == '/';
}

XLineManager *XLine::GetManager()
{
	return ServiceManager::Get()->FindService<XLineManager *>(GetType());
}

void XLineType::Mask::SetField(XLine *s, const Anope::string &value)
{
	Serialize::Field<XLine, Anope::string>::SetField(s, value);
	s->Recache();
}

void XLineManager::RegisterXLineManager(XLineManager *xlm)
{
	XLineManagers.push_back(xlm);
}

void XLineManager::UnregisterXLineManager(XLineManager *xlm)
{
	auto it = std::find(XLineManagers.begin(), XLineManagers.end(), xlm);
	if (it != XLineManagers.end())
		XLineManagers.erase(it);
}

void XLineManager::CheckAll(User *u)
{
	for (XLineManager *xlm : XLineManagers)
		if (xlm->CheckAllXLines(u))
			break;
}

Anope::string XLineManager::GenerateUID()
{
	Anope::string id;

	for (int i = 0; i < 10; ++i)
	{
		char c;
		do
			c = (rand() % 75) + 48;
		while (!isupper(c) && !isdigit(c));
		id += c;
	}

	return id;
}

XLineManager::XLineManager(Module *creator, const Anope::string &xname, char t) : Service(creator, NAME, xname), type(t)
{
}

XLineManager::~XLineManager()
{
	this->Clear();
}

char XLineManager::Type()
{
	return this->type;
}

std::vector<XLine *> XLineManager::GetXLines() const
{
	std::vector<XLine *> xlines;

	for (XLine *x : Serialize::GetObjects<XLine *>())
		if (x->GetType() == this->GetName())
			xlines.push_back(x);

	return xlines;
}

void XLineManager::AddXLine(XLine *x)
{
	x->SetType(this->GetName());
}

XLine* XLineManager::GetEntry(unsigned index)
{
	std::vector<XLine *> xlines = GetXLines();

	if (index >= xlines.size())
		return nullptr;

	return xlines.at(index);
}

void XLineManager::Clear()
{
	for (XLine *x : GetXLines())
		x->Delete();
}

bool XLineManager::CanAdd(CommandSource &source, const Anope::string &mask, time_t expires, const Anope::string &reason)
{
	for (XLine *x : GetXLines())
	{
		if (x->GetMask().equals_ci(mask))
		{
			if (!x->GetExpires() || x->GetExpires() >= expires)
			{
				if (x->GetReason() != reason)
				{
					x->SetReason(reason);
					source.Reply(_("Reason for %s updated."), x->GetMask().c_str());
				}
				else
					source.Reply(_("%s already exists."), mask.c_str());
			}
			else
			{
				x->SetExpires(expires);
				if (x->GetReason() != reason)
				{
					x->SetReason(reason);
					source.Reply(_("Expiry and reason updated for %s."), x->GetMask().c_str());
				}
				else
					source.Reply(_("Expiry for %s updated."), x->GetMask().c_str());
			}

			return false;
		}
		else if (Anope::Match(mask, x->GetMask()) && (!x->GetExpires() || x->GetExpires() >= expires))
		{
			source.Reply(_("%s is already covered by %s."), mask.c_str(), x->GetMask().c_str());
			return false;
		}
		else if (Anope::Match(x->GetMask(), mask) && (!expires || x->GetExpires() <= expires))
		{
			source.Reply(_("Removing %s because %s covers it."), x->GetMask().c_str(), mask.c_str());
			x->Delete();
		}
	}

	return true;
}

XLine* XLineManager::HasEntry(const Anope::string &mask)
{
	for (XLine *x : GetXLines())
		if (x->GetMask().equals_ci(mask))
			return x;

	return nullptr;
}

XLine *XLineManager::CheckAllXLines(User *u)
{
	for (XLine *x : Serialize::GetObjects<XLine *>())
	{
		if (x->GetExpires() && x->GetExpires() < Anope::CurTime)
		{
			this->OnExpire(x);
			x->Delete();
			continue;
		}

		if (this->Check(u, x))
		{
			this->OnMatch(u, x);
			return x;
		}
	}

	return nullptr;
}

void XLineManager::OnExpire(XLine *x)
{
}

