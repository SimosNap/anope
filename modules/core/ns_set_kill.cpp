/* NickServ core functions
 *
 * (C) 2003-2010 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 */

/*************************************************************************/

#include "module.h"

class CommandNSSetKill : public Command
{
 public:
	CommandNSSetKill(const Anope::string &spermission = "") : Command("KILL", 2, 3, spermission)
	{
	}

	CommandReturn Execute(CommandSource &source, const std::vector<Anope::string> &params)
	{
		NickAlias *na = findnick(params[0]);
		if (!na)
			throw CoreException("NULL na in CommandNSSetKill");
		NickCore *nc = na->nc;

		Anope::string param = params[1];
		Anope::string arg = params.size() > 2 ? params[2] : "";

		if (param.equals_ci("ON"))
		{
			nc->SetFlag(NI_KILLPROTECT);
			nc->UnsetFlag(NI_KILL_QUICK);
			nc->UnsetFlag(NI_KILL_IMMED);
			source.Reply(NICK_SASET_KILL_ON, nc->display.c_str());
		}
		else if (param.equals_ci("QUICK"))
		{
			nc->SetFlag(NI_KILLPROTECT);
			nc->SetFlag(NI_KILL_QUICK);
			nc->UnsetFlag(NI_KILL_IMMED);
			source.Reply(NICK_SASET_KILL_QUICK, nc->display.c_str());
		}
		else if (param.equals_ci("IMMED"))
		{
			if (Config->NSAllowKillImmed)
			{
				nc->SetFlag(NI_KILLPROTECT);
				nc->SetFlag(NI_KILL_IMMED);
				nc->UnsetFlag(NI_KILL_QUICK);
				source.Reply(NICK_SASET_KILL_IMMED, nc->display.c_str());
			}
			else
				source.Reply(NICK_SET_KILL_IMMED_DISABLED);
		}
		else if (param.equals_ci("OFF"))
		{
			nc->UnsetFlag(NI_KILLPROTECT);
			nc->UnsetFlag(NI_KILL_QUICK);
			nc->UnsetFlag(NI_KILL_IMMED);
			source.Reply(NICK_SASET_KILL_OFF, nc->display.c_str());
		}
		else
			this->OnSyntaxError(source, "KILL");

		return MOD_CONT;
	}

	bool OnHelp(CommandSource &source, const Anope::string &)
	{
		source.Reply(NICK_HELP_SET_KILL);
		return true;
	}

	void OnSyntaxError(CommandSource &source, const Anope::string &)
	{
		SyntaxError(source, "SET KILL", Config->NSAllowKillImmed ? NICK_SET_KILL_IMMED_SYNTAX : NICK_SET_KILL_SYNTAX);
	}

	void OnServHelp(CommandSource &source)
	{
		source.Reply(NICK_HELP_CMD_SET_KILL);
	}
};

class CommandNSSASetKill : public CommandNSSetKill
{
 public:
	CommandNSSASetKill() : CommandNSSetKill("nickserv/saset/kill")
	{
	}

	bool OnHelp(CommandSource &source, const Anope::string &)
	{
		source.Reply(NICK_HELP_SASET_KILL);
		return true;
	}

	void OnSyntaxError(CommandSource &source, const Anope::string &)
	{
		SyntaxError(source, "SASET KILL", Config->NSAllowKillImmed ? NICK_SASET_KILL_IMMED_SYNTAX : NICK_SASET_KILL_SYNTAX);
	}

	void OnServHelp(CommandSource &source)
	{
		source.Reply(NICK_HELP_CMD_SET_KILL);
	}
};

class NSSetKill : public Module
{
	CommandNSSetKill commandnssetkill;
	CommandNSSASetKill commandnssasetkill;

 public:
	NSSetKill(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator)
	{
		this->SetAuthor("Anope");
		this->SetType(CORE);

		Command *c = FindCommand(NickServ, "SET");
		if (c)
			c->AddSubcommand(this, &commandnssetkill);

		c = FindCommand(NickServ, "SASET");
		if (c)
			c->AddSubcommand(this, &commandnssasetkill);
	}

	~NSSetKill()
	{
		Command *c = FindCommand(NickServ, "SET");
		if (c)
			c->DelSubcommand(&commandnssetkill);

		c = FindCommand(NickServ, "SASET");
		if (c)
			c->DelSubcommand(&commandnssasetkill);
	}
};

MODULE_INIT(NSSetKill)
