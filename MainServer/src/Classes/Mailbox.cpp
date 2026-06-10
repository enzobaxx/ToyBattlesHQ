#include "../../include/Classes/Mailbox.h"

#include <algorithm>

namespace Main
{
	namespace Classes
	{
		void Mailbox::addMailboxReceived(const Main::Structures::Mailbox& mailbox)
		{
			m_mailboxReceived.push_back(mailbox);
		}

		void Mailbox::addGiftboxReceived(const Main::Structures::Giftbox& giftbox)
		{
			m_giftboxReceived.push_back(giftbox);
		}

		void Mailbox::addMailboxSent(const Main::Structures::Mailbox& mailbox)
		{
			m_mailboxSent.push_back(mailbox);
		}

		bool Mailbox::deleteSentMailbox(std::uint32_t timestamp)
		{
			auto it = std::find_if(m_mailboxSent.begin(), m_mailboxSent.end(), [timestamp](const auto& mailbox) {
				return mailbox.timestamp == timestamp;
				});
			if (it != m_mailboxSent.end())
			{
				m_mailboxSent.erase(it);
				return true;
			}
			return false;
		}

		bool Mailbox::deleteReceivedMailbox(std::uint32_t timestamp)
		{
			auto it = std::find_if(m_mailboxReceived.begin(), m_mailboxReceived.end(), [timestamp](const auto& mailbox) {
				return mailbox.timestamp == timestamp;
				});
			if (it != m_mailboxReceived.end())
			{
				m_mailboxReceived.erase(it);
				return true;
			}
			return false;
		}

		const std::vector<Main::Structures::Mailbox>& Mailbox::getMailboxReceived() const
		{
			return m_mailboxReceived;
		}

		const std::vector<Main::Structures::Mailbox>& Mailbox::getMailboxSent() const
		{
			return m_mailboxSent;
		}

		const std::vector<Main::Structures::Giftbox>& Mailbox::getGiftboxReceived() const
		{
			return m_giftboxReceived;
		}

		void Mailbox::deleteGiftbox(std::uint32_t timestamp)
		{
			auto it = std::remove_if(m_giftboxReceived.begin(), m_giftboxReceived.end(), [timestamp](const auto& giftbox) {
				return giftbox.timestamp == timestamp;
				});

			if (it != m_giftboxReceived.end())
			{
				m_giftboxReceived.erase(it, m_giftboxReceived.end());
			}
		}

		void Mailbox::setMailbox(const std::vector<Main::Structures::Mailbox>& mailbox, bool sent)
		{
			if (sent) m_mailboxSent = mailbox;
			else m_mailboxReceived = mailbox;
		}

		std::optional<std::uint32_t> Mailbox::getItemIdFromGiftbox(std::uint32_t timestamp) const
		{
			for (const auto& currentGiftbox : m_giftboxReceived)
			{
				if (currentGiftbox.timestamp == timestamp)
				{
					return currentGiftbox.id;
				}
			}
			return std::nullopt;
		}

		std::optional<Main::Structures::Giftbox> Mailbox::getGiftbox(std::uint32_t timestamp) const
		{
			for (const auto& currentGiftbox : m_giftboxReceived)
			{
				if (currentGiftbox.timestamp == timestamp)
				{
					return currentGiftbox;
				}
			}
			return std::nullopt;
		}

		void Mailbox::setReceivedGiftboxes(const std::vector<Main::Structures::Giftbox>& giftbox)
		{
			m_giftboxReceived = giftbox;
		}
	}
}
