#ifndef MAILBOX_CLASS_H
#define MAILBOX_CLASS_H

#include "../Structures/Item/ItemId.h"
#include "../Structures/Item/MainItemSerialInfo.h"
#include "../Structures/Mailbox.h"

#include <vector>
#include <optional>
#include <cstdint>

namespace Main
{
	namespace Classes
	{
		class Mailbox
		{
		private:
			std::vector<Main::Structures::Mailbox> m_mailboxReceived{};
			std::vector<Main::Structures::Mailbox> m_mailboxSent{};
			std::vector<Main::Structures::Giftbox> m_giftboxReceived{};

		public:
			void addMailboxReceived(const Main::Structures::Mailbox& mailbox);
			void addGiftboxReceived(const Main::Structures::Giftbox& giftbox);
			void addMailboxSent(const Main::Structures::Mailbox& mailbox);
			bool deleteSentMailbox(std::uint32_t timestamp);
			bool deleteReceivedMailbox(std::uint32_t timestamp);
			const std::vector<Main::Structures::Mailbox>& getMailboxReceived() const;
			const std::vector<Main::Structures::Mailbox>& getMailboxSent() const;
			const std::vector<Main::Structures::Giftbox>& getGiftboxReceived() const;
			void deleteGiftbox(std::uint32_t timestamp);
			void setMailbox(const std::vector<Main::Structures::Mailbox>& mailbox, bool sent);
			std::optional<std::uint32_t> getItemIdFromGiftbox(std::uint32_t timestamp) const;
			std::optional<Main::Structures::Giftbox> getGiftbox(std::uint32_t timestamp) const;
			void setReceivedGiftboxes(const std::vector<Main::Structures::Giftbox>& giftbox);
		};
	}
}

#endif
