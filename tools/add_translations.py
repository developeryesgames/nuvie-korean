#!/usr/bin/env python3
"""Add Geoffrey and Nystul translations to dialogues_ko.txt"""

translations = """
# Geoffrey (NPC 7) - Castle Britannia Guard Captain
7|"a tall, handsome man."|"키 크고 잘생긴 남자다."
7|"Thou hadst best speak to Lord British before aught else."|"다른 일보다 먼저 로드 브리티시와 이야기하는 게 좋겠어."
7|"I'm glad to see thee, $P. Perhaps thou canst prevail where others could not."|"만나서 반가워, $P. 다른 이들이 실패한 곳에서 네가 성공할 수 있을 거야."
7|"I sent a party of ten to recapture the Shrine of Compassion from the gargoyles."|"가고일에게서 연민의 성소를 되찾으려고 열 명을 보냈어."
7|"Alas, they failed dismally."|"안타깝게도 처참하게 실패했지."
7|"The survivors are recuperating in the town of Cove."|"생존자들은 코브 마을에서 회복 중이야."
7|"Thou wouldst do well to speak with them first."|"먼저 그들과 이야기해 보는 게 좋을 거야."
7|"Mayhap they learned something which might aid thee."|"네게 도움이 될 만한 걸 알게 되었을 수도 있거든."
7|"I must confess I fear the worst."|"솔직히 최악의 상황이 두려워."
7|"The gargoyles are such powerful foes, and they are spreading so fast..."|"가고일은 너무 강력한 적이고, 너무 빠르게 퍼지고 있어..."
7|"Perhaps the end of the realm is nigh."|"어쩌면 왕국의 종말이 가까워진 건지도 몰라."
7|"Good luck, and my prayers go with thee."|"행운을 빌어, 내 기도가 함께할 거야."
7|"Best not waste time talking..."|"말하느라 시간 낭비하지 않는 게 좋겠어..."
7|"Who knows what acts of villainy the gargoyles are comitting even as we speak?"|"지금 이 순간에도 가고일이 무슨 악행을 저지르고 있을지 누가 알겠어?"
7|"I hope you've had a chance to visit Cove and speak with Gertan."|"코브에 가서 게르탄과 이야기할 기회가 있었으면 좋겠어."

# Nystul (NPC 6) - Court Mage
6|"a concerned looking mage."|"걱정스러운 표정의 마법사다."
6|"Hail to thee $G, and well met."|"$G님, 만나서 반갑습니다."
6|"'Twas I who learned of thy peril through my mystic arts, so that aid might be sent unto thee."|"신비로운 기술로 당신의 위험을 알게 된 것은 저였습니다. 그래서 도움을 보낼 수 있었지요."
6|"Iolo, I saw that thou didst find a book."|"이올로, 책을 발견했다고 들었네."
6|"Might I examine it?"|"내가 살펴봐도 되겠나?"
6|"I no longer have the book, milord."|"더 이상 그 책이 없습니다, 나리."
6|"That is too bad. I had hoped thou might take it to Mariah and have her translate it."|"안타깝군. 마리아에게 가져가서 번역하게 하려 했는데."
6|"Certainly, milord. Perhaps thou canst make better sense of it than I."|"물론이죠, 나리. 저보다 더 잘 이해하실 수 있을 겁니다."
6|"Strange..."|"이상하군..."
6|"This has a picture on its cover of a gargoyle standing with one foot on the chest of a slain human."|"표지에 가고일이 죽은 인간의 가슴에 발을 올리고 서 있는 그림이 있네."
6|"This is interesting."|"흥미롭군."
6|"It's written in a language I know not."|"내가 모르는 언어로 쓰여 있어."
6|"Take it to Mariah at the Lycaeum, the finest scribe on the great Council of Wizards."|"리시움에 있는 마리아에게 가져가게. 대마법사 의회 최고의 서기관이지."
6|"She has studied many languages, and perhaps she can decipher this book for thee."|"그녀는 많은 언어를 공부했으니, 아마 이 책을 해독할 수 있을 거야."
6|"One more thing, Avatar..."|"한 가지 더, 아바타여..."
6|"I noticed that thou didst arrive through a red gateway."|"네가 붉은 관문을 통해 도착했다는 걸 알아챘네."
6|"Dost thou have the stone that opened the gate?"|"관문을 연 돌을 가지고 있나?"
6|"From whence could it have come? The gargoyles, perhaps?"|"어디서 온 것일까? 가고일에게서인가?"
6|"Best ask Lord British about it."|"로드 브리티시에게 물어보는 게 좋겠네."
6|"I believe he has some knowledge of such items."|"그분이 그런 물건에 대해 좀 알고 계실 거야."
6|"Such items are quite rare."|"그런 물건은 꽤 희귀하지."
6|"Indeed, the only one I have ever seen is that which Lord British himself possesses."|"사실 내가 본 것 중 유일한 것은 로드 브리티시께서 직접 가지고 계신 것뿐이야."
6|"Hast thou gone to the Lycaeum and shown Mariah the book? 'Tis most important."|"리시움에 가서 마리아에게 책을 보여줬나? 정말 중요한 일이야."
6|"There is naught else I can help thee with at this time."|"지금은 더 도울 수 있는 게 없네."
"""

def main():
    filepath = "d:/proj/Ultima6/nuvie/data/translations/korean/dialogues_ko.txt"

    with open(filepath, 'a', encoding='utf-8') as f:
        for line in translations.strip().split('\n'):
            line = line.strip()
            if line and not line.startswith('#'):
                f.write(line + '\n')

    print("Added Geoffrey and Nystul translations!")

if __name__ == "__main__":
    main()
