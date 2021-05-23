#include "UcData.h"

using namespace std::string_view_literals;


const uc::LangLife uc::langLifeInfo[static_cast<int>(EcLangLife::NN)] {
    { {} },
    { u8"живая <i>(основным языкам ничего не угрожает, как кириллица)</i>"sv },
    { u8"в опасности <i>(задавлена соседними языками, как чероки, и/или социальными потрясениями, как сирийская)</i>"sv },
    { u8"мёртвая <i>(скоро потеряет/недавно потеряла носителей, как нюй-шу)</i>"sv },
    { u8"историческая <i>(давно мертва, как глаголица)</i>"sv },
    { u8"новая <i>(появилась в новейшее время, как нко)</i>"sv },
    { u8"возрождённая <i>(как ахом)</i>"sv },
    //{ u8"Ошибка"sv },
};


const uc::WritingDir uc::writingDirInfo[static_cast<int>(EcWritingDir::NN)] {
    { {} },
    { u8"→"sv },
    { u8"←"sv },
    { u8"→ <i>(исторически ← по столбцам)</i>"sv },
    //{ u8"Ошибка"sv },
};


const uc::ScriptType uc::scriptTypeInfo[static_cast<int>(EcScriptType::NN)] {
    { {} },
    { u8"неизвестно"sv },
    { u8"алфавитная <i>(гласные и согласные звуки, как кириллица)</i>"sv },
    { u8"консонантная <i>(согласные звуки, как иврит)</i>"sv },
    { u8"слоговая <i>(как японские катакана/хирагана)</i>"sv },
    { u8"алфавитно-слоговая <i>(как иберийский)</i>"sv },
    { u8"абугида <i>(слоговая, у сходных слогов похожие символы)</i>, "sv""
        "кодируется монолитными символами\u00A0— как эфиопский"sv },
    { u8"абугида <i>(слоговая, у сходных слогов похожие символы)</i>, "sv
        u8"кодируется по-алфавитному, из согласного и метки-огласовки\u00A0— как деванагари"sv },
    { u8"иероглифическая <i>(символ означает понятие, как китайский)</i>"sv },
    { u8"слогоиероглифическая <i>(как линейное Б)</i>"sv },
    { u8"код <i>(записывает символы другого языка, как код Брайля)</i>"sv },
    { u8"символы <i>(как ноты)</i>"sv },
    { u8"настольная игра <i>(как игральные карты)</i>"sv },
    { u8"эмодзи <i>(японские SMS-картинки)</i>"sv },
    //{ u8"Ошибка"sv },
};


const uc::Version uc::versionInfo[static_cast<int>(uc::EcVersion::NN)] {
    //{ "1.0"sv,  1991 },
    //{ "1.0.1"sv,1992 },
    { "1.1"sv,  1993 },
    { "2.0"sv,  1996 },
    { "2.1"sv,  1998 },
    { "3.0"sv,  1999 },
    { "3.1"sv,  2001 },
    { "3.2"sv,  2002 },
    { "4.0"sv,  2003 },
    { "4.1"sv,  2005 },
    { "5.0"sv,  2006 },
    { "5.1"sv,  2008 },
    { "5.2"sv,  2009 },
    { "6.0"sv,  2010 },
    { "6.1"sv,  2012 },
    { "6.2"sv,  2012 },
    { "6.3"sv,  2013 },
    { "7.0"sv,  2014 },
    { "8.0"sv,  2015 },
    { "9.0"sv,  2016 },
    { "10.0"sv, 2017 },
    { "11.0"sv, 2018 },
    { "12.0"sv, 2019 },
    { "12.1"sv, 2019 },
    { "13.0"sv, 2020 },
    //{ "14.0"sv, 2021 },       //check for equal number
};


const uc::Category uc::categoryInfo[static_cast<int>(uc::EcCategory::NN)] {
    { UpCategory::CONTROL,      "Cc"sv,     u8"Управляющий"sv,
            u8"В первые годы, когда часто требовалось повторить сессию обмена вручную (перфоратором или консолью), "sv
            u8"управляющие символы играли роль форматов данных и сетевых протоколов. "sv
            u8"В настоящее время используются немногие из них:<br>"sv
            u8"• нулевой символ 00;<br>"sv
            u8"• шаг назад (забой) 08;<br>"sv
            u8"• табуляция 09;<br>"sv
            u8"• перевод строки 0A;<br>"sv
            u8"• прогон листа 0C;<br>"sv
            u8"• возврат каретки 0D;<br>"sv
            u8"• конец файла 1A;<br>"sv
            u8"• Escape 1B."sv },
    { UpCategory::FORMAT,       "Cf"sv,     u8"Форматирующий"sv,
            u8"Используются в Юникоде для управления различными его алгоритмами, в первую очередь двунаправленным письмом "sv
            u8"(когда соседствуют текст слева направо и справа налево) и разбиением текста на строки."sv },
    // PRIVATE_USE,     -- unused as Unicodia has nothing to tell
    // SURROGATE,       -- unused as Unicodia has nothing to tell
    { UpCategory::LETTER,       "Ll"sv,     u8"Буква/строчная"sv,
            u8"Письмо «минускул», появившееся в раннее средневековье для экономии дорогого пергамента, "sv
            u8"превратилось в строчные буквы и сделало латиницу очень выразительным алфавитом." },
    { UpCategory::LETTER,       "Lm"sv,     u8"Буква/модифицирующая"sv,
            u8"Такие «недобуквы», приделывающие к букве оттенки смысла, чаще всего встречаются в фонетике. "sv
            u8"Так, модифицирующей буквой является штрих мягкости xʹ, в отличие от похожего математического штриха x′."sv },
    { UpCategory::LETTER,       "Lo"sv,     u8"Буква/другая"sv,
            u8"Символы различных письменностей, где нет деления на заглавные и строчные буквы. "sv
            u8"Например, иврит, арабская вязь и китайские иероглифы. Также особые символы вроде "sv
            u8"мужского/женского порядкового знака романских языков (1º\u00A0— первый, 1ª\u00A0— первая)."sv },
    { UpCategory::LETTER,       "Lt"sv,     u8"Буква/смешанный регистр"sv,
            u8"Символы-диграфы, состоящие из заглавной и строчной букв. "sv
            u8"Так, в хорватской латинице есть диграф ǈ. "sv
            u8"Сербский аналог Љ не является диграфом и потому смешанного регистра не имеет."sv },
    { UpCategory::LETTER,       "Lu"sv,     u8"Буква/заглавная"sv,
            u8"Те письменности, которые приняли маленькие (строчные) буквы, буквы старой формы стали "sv
            u8"называть большими, заглавными или прописными."sv },
    { UpCategory::MARK,         "Mc"sv,     u8"Метка/протяжённая"sv,
            u8"Протяжённые (обладающие шириной) комбинирующие метки встречаются в некоторых "sv
            u8"языках Юго-Восточной Азии: деванагари, бенгальском, каннаде, хангыле…"sv },
    { UpCategory::MARK,         "Me"sv,     u8"Метка/охватывающая"sv,
            u8"Охватывающие метки используются в древнерусских буквенных цифрах (А҈). "sv
            u8"Также существуют охватывающие квадрат, круг и другие фигуры. "sv
            u8"Ни один проверенный типографский движок на Windows 10 20H2 (GDI, Cairo, Skia) не поддерживает подобные символы идеально, "sv
            u8"но на хороших шрифтах вроде DejaVu результат очень неплох." },
    { UpCategory::MARK,         "Mn"sv,     u8"Метка/непротяжённая"sv,
            u8"Непротяжённые метки (например, знак ударе́ния) устроены как символы нулевой ширины, отсюда название. "sv
            u8"В хороших шрифтах дизайнер вручную помещает метки на наиболее распространённые буквы. "sv
            u8"Но если подобной комбинации не предусмотрели (8́), символы накладываются как попало, и результат обычно плох."sv },
    { UpCategory::NUMBER,       "Nd"sv,     u8"Числовой/десятичный"sv,
            u8"Люди считают десятками, поскольку у них десять пальцев. Слова «цифра» и «палец» во многих языках близки. "sv
            u8"Изобретённая в Индии позиционная система счисления используется всем миром, однако "sv
            u8"в языках Ближнего Востока и Юго-Восточной Азии цифры бывают причудливые."sv },
    { UpCategory::NUMBER,       "Nl"sv,     u8"Числовой/буквенный"sv,
            u8"Римские, китайские и другие цифры, основанные на буквах. Древнерусские также основаны на буквах, "sv
            u8"но в Юникоде для этого используются обычные А, Б, Г… с комбинирующими метками."sv },
    { UpCategory::NUMBER,       "No"sv,     u8"Числовой/другой"sv,
            u8"Архаичные системы счисления, монолитные дроби, верхние и нижние индексы, цифры в кругах, "sv
            u8"сокращения для больших чисел и другие цифровые символы."sv },
    { UpCategory::PUNCTUATION,  "Pc"sv, u8"Знак препинания/соединитель"sv,
            u8"Небольшая группа символов. Наиболее известный из них\u00A0— знак подчёркивания."sv },
    { UpCategory::PUNCTUATION,  "Pd"sv,     u8"Знак препинания/черта"sv,
            u8"Дефисы и тире. Минус также является чертой, но отнесён к математическим знакам."sv },
    { UpCategory::PUNCTUATION,  "Pe"sv,     u8"Знак препинания/закрывающая скобка"sv,
            u8"Скобки отнечены к отдельной категории, потому что играют важную роль в двунаправленном алгоритме."sv },
    { UpCategory::PUNCTUATION,  "Pf"sv,     u8"Знак препинания/конечный"sv,
            u8"В основном кавычки."sv },
    { UpCategory::PUNCTUATION,  "Pi"sv,     u8"Знак препинания/начальный"sv,
            u8"В основном кавычки."sv },
    { UpCategory::PUNCTUATION,  "Po"sv,     u8"Знак препинания/другой"sv,
            u8"Точка, запятая, процент, маркер списка и многое что ещё."sv },
    { UpCategory::PUNCTUATION,  "Ps"sv,     u8"Знак препинания/открывающая скобка"sv,
            u8"Скобки отнечены к отдельной категории, потому что играют важную роль в двунаправленном алгоритме."sv },
    { UpCategory::SYMBOL,       "Sc"sv,     u8"Символ/валютный"sv,
            u8"Валютный символ\u00A0— важная часть имиджа страны, и даже у монголов есть тугрик ₮, напоминающий могильный крест. "sv
            u8"Артемий Лебедев в конце 1990-х говорил, что рублю не нужен особый знак, "sv
            u8"но впоследствии именно его пиар сделал знак ₽ официальным."sv },
    { UpCategory::SYMBOL,       "Sk"sv,     u8"Символ/модифицирующий"sv,
            u8"Символы вроде крышки ^, внешне совпадающие с комбинирующими метками, но лишённые особых свойств. "sv
            u8"А также некоторые другие символы." },
    { UpCategory::SYMBOL,       "Sm"sv,     u8"Символ/математический"sv,
            u8"Изначально для математики использовались несколько разных систем, наиболее известные из них\u00A0— TᴇX (читается «тех») "sv
            u8"и MathType (он же Microsoft Equation). Юникод даёт надежду, что в компьютерной математике появится какая-то стандартизация\u00A0— "sv
            u8"а также ограниченная возможность писать формулы в системах общего назначения вроде интернет-форумов."sv },
    { UpCategory::SYMBOL,       "So"sv,     u8"Символ/другой"sv,
            u8"Юникод содержит множество разных символов, в том числе коммерческие, технические и эмодзи."sv },
    { UpCategory::SEPARATOR,    "Zl"sv,     u8"Разделитель/строк"sv,
            u8"Единственный символ 2028"sv },
    { UpCategory::SEPARATOR,    "Zp"sv,     u8"Разделитель/абзацев"sv,
            u8"Единственный символ 2029"sv },
    { UpCategory::SEPARATOR,    "Zs"sv,     u8"Разделитель/пробел"sv,
            u8"В хорошо проработанной типографике "sv
            u8"пустого места столько, сколько нужно: 146%\u00A0— мало, 146\u00A0%\u00A0— много, "sv
            u8"146<span style='font-size:3pt'>\u00A0</span>%\u00A0— самое то. Потому и пробелы бывают разные. "sv
            u8"Некоторые из них неразрывные: не расширяются при выключке, не переносятся на другую строку."sv },
    //{ u8"Ошибка"sv },     //check for equal number
};



const uc::Script uc::scriptInfo[static_cast<int>(uc::EcScript::NN)] {
    { "Zyyy"sv, EcScriptType::NONE, EcLangLife::NOMATTER, EcWritingDir::NOMATTER,
            "Нет"sv, {}, {}, "Символы вне письменности."sv },
    { "Adlm"sv, EcScriptType::ALPHABET, EcLangLife::NEW, EcWritingDir::RTL,
        u8"Адлам"sv, u8"конец 1980-х",
        u8"фулá <i>(семейство языков Западной Африки)</i>"sv,
        u8"<p>Алфавит придуман братьями Ибрагима и Абдулайе Барри, чтобы лучше передавать языки фулá, чем латиница или арабский. "
                u8"Новая письменность прижилась, и её учат в школах Гвинеи, Либерии и других стран, локализован Android.</p>"sv
            u8"<p>Алфавит назван по первым четырём буквам: A, D, L, M. "sv
                u8"Эти буквы означают «Alkule Dandayɗe Leñol Mulugol»\u00A0— «алфавит, защищающий народы от исчезновения».</p>"sv },
    { "Aghb"sv, EcScriptType::ALPHABET, EcLangLife::HISTORICAL, EcWritingDir::LTR,
        u8"Агванская"sv, u8"II век до н.э.",
        u8"агванский <i>(язык Кавказской Албании)</i>"sv,
        u8"<p>Считается, что создана Месропом Маштоцем, создателем армянского алфавита\u00A0— впрочем, это не так: "sv
                u8"Маштоц жил 362–440, и непонятно, какова была его роль. "sv
                u8"Упоминалась с конца XIX века. Окончательно открыта в 1937 году советскими арменоведами. "sv
                u8"Первая расшифровка вышла в 2009 году.</p>"sv },
    { "Ahom"sv, EcScriptType::ABUGIDA_COMBINING, EcLangLife::REVIVED, EcWritingDir::LTR,
        u8"Ахом"sv, u8"XIII век",
        u8"тайско-ахомский"sv,
        u8"<p>Тайцы, переселившиеся в долину реки Брахмапутра, создали письменность на основе тогдашней индийской абугиды. "sv
                u8"К XIX веку язык окончательно заместился ассамским. Возрождается с 1954 года.</p>"sv },
    { "Arab"sv, EcScriptType::CONSONANT, EcLangLife::ALIVE, EcWritingDir::RTL,
        u8"Арабская"sv, u8"IV—VI век",
        u8"арабский, персидский, урду, уйгурский, пуштунский…"sv,
        u8"<p>Письменность развилась из сирийской. Арабский язык тесно связан с исламом; на этом языке написан Коран (610—632). "sv
                u8"Арабский халифат насаждал как ислам, так и вязь. "sv
                u8"Многие исламские народы (турки, казахи, башкиры) пользовались арабицей до начала XX века.</p>"
            u8"<p>Компьютерная арабица осложняется написанием арабских букв: у каждой есть обособленная, начальная, средняя и конечная форма. "sv
                u8"В обычном тексте предпочтительнее «общая» форма буквы, подстраивающаяся под положение в слове. "sv
                u8"Но если нужна конечная форма в обособленной букве, в Юникоде есть и «жёсткие» варианты.</p>"sv },
    { "Armi"sv, EcScriptType::CONSONANT, EcLangLife::HISTORICAL, EcWritingDir::RTL,
        u8"Имперская арамейская"sv, u8"VII в. до н.э.",
        u8"имперский арамейский <i>(также канцелярский арамейский\u00A0— язык Персии 500—329 до н.э.)</i>"sv,
        u8"<p>Использовался в Ахеменидской империи как книжный язык. Был кодифицирован настолько, что крайне сложно опознать "sv
                u8"время и место конкретного документа. С завоеванием Персии Александром Македонским началась "sv
                u8"фрагментация языка и дальнейшее формирование сирийских языков (предков арабского) и иудейских (предков иврита).</p>"sv },
    { "Armn"sv, EcScriptType::ALPHABET, EcLangLife::ALIVE, EcWritingDir::LTR,
        u8"Армянская"sv, u8"405",
        u8"армянский"sv,
        u8"<p>Изобретён учёным и священником Месропом Маштоцем (362—440). Непонятно, брался ли какой-то алфавит за основу "sv
                u8"(возможно, несохранившиеся древнеармянские буквы). Алфавит тесно связан с распространением христианства в Аримении. "sv
                u8"В XII веке добавились буквы Օ и Ֆ.</p>"sv
            u8"<p>Считается, что армянская литература богаче среднеперсидской (доисламской), потому что армянский алфавит проще "sv
                u8"манихейской вязи.</p>"sv },
    { "Beng"sv, EcScriptType::ABUGIDA_COMBINING, EcLangLife::ALIVE, EcWritingDir::LTR,
        u8"Бенгальская"sv, u8"XV в.",
        u8"бенгальский, ассамский, санскрит"sv,
        u8"<p>Относится к северной ветви индийского письма. По устройству и представлению в Юникоде похожа на деванагари. "sv
                u8"Гласная по умолчанию не «а», как в деванагари, а «о».</p>"sv },
    { "Cyrl"sv, EcScriptType::ALPHABET, EcLangLife::ALIVE, EcWritingDir::LTR,
        u8"Кириллица"sv, u8"конец IX в.",
        u8"русский, украинский, белорусский, русинский, болгарский, македонский, сербохорватский (Сербия), "sv
            u8"казахский, киргизский, таджикский, языки народов России"sv,
        u8"<p>До 900 года византийские монахи Кирилл и Мефодий придумали славянскую письменность; неизвестно, была ли это кириллица. "sv
                u8"Прообразом кириллицы стало византийское письмо унциал для греческого языка, с добавлением звуков, отсутствующих в греческом. "
                u8"Старшинство кириллицы и глаголицы (появившейся тогда же) спорно, большинство учёных считают, что "sv
                u8"глаголицу создал Кирилл, а кириллицу\u00A0— его ученик Климент Охридский, работавший в Болгарии.</p>"sv
            u8"<p>Кириллица распространилась среди южных славян и проникла на Русь с крещением. "sv
                u8"В странах, попавших под влияние Рима, кириллицу прямо запрещали.</p>"sv
            u8"<p>Современная кириллица восходит к гражданскому шрифту, введённому Петром\u00A0I. Шрифт стал компромиссом между сторонниками "sv
                u8"традиций и западниками.</p>"sv
            u8"<p>СССР сделал кириллическую письменность для многих языков союзных республик. С развалом СССР на латиницу перешли "sv
                u8"Азербайджан, Молдавия, Туркмения, Узбекистан.</p>"sv },
    { "Deva"sv, EcScriptType::ABUGIDA_COMBINING, EcLangLife::ALIVE, EcWritingDir::LTR,
        u8"Деванагари"sv, u8"X век",
        u8"хинди, санскрит и другие языки Индии"sv,
        u8"<p>Деванагари (буквально «язык божественного города») развился из письма брахми и стал алфавитом для многих языков Индии. "sv
                u8"Особенность деванагари\u00A0— все символы свисают с горизонтальной черты.</p>"sv
        u8"<p>Каждый символ означает слог с гласной «а». Чтобы отобразить другие слоги, надо добавить огласовку: "sv
                u8"<nobr>ка <font size='+2'>क</font> + и<font size='+2'> ि</font> = ки <font size='+2'>कि</font></nobr>. "sv
                u8"Чтобы получить простую согласную, надо добавить знак «вирама» («убийца»): к <font size='+2'>क्</font>.</p>"sv },
    { "Grek"sv, EcScriptType::ALPHABET, EcLangLife::ALIVE, EcWritingDir::LTR,
        u8"Греческая"sv, u8"IX век до н.э.",
        u8"греческий"sv,
        u8"<p>Греческий сделан из финикийского алфавита без оглядки на раннегреческие системы\u00A0— линейное письмо Б и кипрское. "sv
                u8"Названия букв изменились мало, но перестали что-то значить: алеф=бык\u00A0→ альфа. Первый известный истинный алфавит, "sv
                u8"кодирующий как согласные звуки, так и гласные.</p>"sv
        u8"<p>Из греческого прошли многие письменности мира, включая латиницу и кириллицу.</p>"sv },
    { "Hang"sv, EcScriptType::ABUGIDA_MONOLITH, EcLangLife::ALIVE, EcWritingDir::LTR,
        u8"Хангыль"sv, u8"1443",
        u8"корейский"sv,
        u8"<p>Хангыль (мужской род!)\u00A0— корейское алфавитно-слоговое письмо. В нём 51 чамо (буква) объединяется в группы приблизительно по слогам.</p>"
        u8"<p>Придуман группой учёных под руководством царя Седжона Великого, считавшего, что китайские иероглифы плохо передают корейский язык "sv
                u8"и сложны для народа. Образованная публика смотрела на хангыль надменно, "sv
                u8"считая женским, детским или народным письмом. Последующие цари даже запрещали хангыль. Возрождение началось в XX веке, "sv
                u8"официальным стал в 1945.</p>"sv },
    { "Hani"sv, EcScriptType::HIEROGLYPH, EcLangLife::ALIVE, EcWritingDir::LTR_CJK,
        u8"Китайские иероглифы"sv, u8"около 2000 до н.э.",
        u8"китайский, японский, ранее вьетнамский и корейский"sv,
        u8"<p>Первые пиктограммы относятся к VI\u00A0тысячелетию до н.э., впоследствии они видоизменились под письмо кистью и тушью. "sv
                u8"Уже в I\u00A0тысячелетии до н.э. люди стали отходить от иероглифов\u00A0— в Китае культ традиции и многонациональный народ "sv
                u8"законсервировали архаичную письменность.<p>"sv
            u8"<p>В III\u00A0веке иероглифы проникли в Японию. У многих иероглифов есть как китайское, так и японское прочтение: "sv
                u8"например, китайское прочтение «ниндзя» и японское «синоби».</p>"sv
            u8"<p>В Китае, Тайване, Японии и Корее иероглифы отличаются, Юникод эти различия не кодирует, "sv
                u8"объединяя их все в один блок CJK (<i>Chinese, Japanese, Korean</i>).</p>"sv
            u8"<p>Вьетнам отказался от иероглифов в 1945. Северная Корея не использует иероглифы, Южная использует крайне редко.</p>"sv },
    { "Hebr"sv, EcScriptType::CONSONANT, EcLangLife::ALIVE, EcWritingDir::RTL,
        u8"Иврит"sv, u8"VI—II в. до н.э.",
        u8"иврит, ладино, идиш, караимский, крымчакский"sv,
        u8"<p>Развился из арамейской письменности, и ко II\u00A0в. до н.э. приобрёл почти современный вид.</p>"sv
            u8"<p>Записывает только согласные буквы, но четыре буквы <font size='+1'>אהוי</font> могут означать гласные. "sv
            u8"С той же целью иногда используют огласовки\u00A0— точки над буквами.</p>"sv },
    { "Latn"sv, EcScriptType::ALPHABET, EcLangLife::ALIVE, EcWritingDir::LTR,
        u8"Латиница"sv, u8"I тысячелетие до н.э.",
        u8"большинство языков западного и тюркского мира (включая Восточную Европу, Прибалтику, Молдавию), Чёрной Африки, Океании; "sv
            u8"вьетнамский, карельский, курдский, эсперанто"sv,
        u8"<p>Латиница\u00A0— древнегреческое письмо, адаптированное около VII\u00A0в. до нашей эры "sv
                u8"для записи этрусских говоров, впоследствии ставших латинском языком.</p>"sv
            u8"<p>В средние века к латинице добавились буквы J, V и W. Минускульное письмо, изобретённое для экономии дорогого пергамента, "sv
                u8"превратилось в строчные буквы.</p>"sv
            u8"<p>Латинский язык давно мёртв, но широкая территория Римской империи, миссионерская деятельность Папского престола "sv
                u8"и Великие географические открытия, совершённые западным миром, сделали латиницу основным алфавитом мира. "sv
                u8"Латиница используется в математике, медицине, фонетике, программировании.</p>"sv
            u8"<p>С развалом СССР на латиницу перешли Азербайджан, Молдавия, Туркмения, Узбекистан.</p>" },
    { "Mand"sv, EcScriptType::CONSONANT, EcLangLife::ENDANGERED, EcWritingDir::RTL,
        u8"Мандейская"sv, u8"II—VII в.",
        u8"мандейский <i>(Иран и Ирак)</i>"sv,
        u8"<p>Произошла из арамейского или парфянского письма. Используется так называемыми «болотными арабами», живущими у слияния рек "sv
                u8"Тигр и Евфрат и исповедующими мандеизм, необычную гностическую религию. Их количество в Ираке быстро уменьшается "sv
                u8"с 45&nbsp;000 (1996) до 5000 (2007), около 60&nbsp;тыс. разбросаны по миру, и, вероятно, на правах беженцев они быстро исчезнут.</p>"sv },
    { "Nkoo"sv, EcScriptType::ALPHABET, EcLangLife::NEW, EcWritingDir::RTL,
        u8"Нко"sv, u8"1949",
        u8"языки манден <i>(Западная Африка)</i>"sv,
        u8"<p>Алфавит создал гвинейский писатель Соломана Канте в 1949 году. Применяется в основном в Гвинее, Кот-д’Ивуаре и Мали. "
            u8"«N’ko» означает «я говорю».</p>"sv },
    { "Samr"sv, EcScriptType::CONSONANT, EcLangLife::ENDANGERED, EcWritingDir::RTL,
        u8"Самаритянская"sv, u8"около 600—200 до н.э.",
        u8"иврит, самаритянский арамейский"sv,
        u8"<p>Происходит из палеоеврейского письма. По Библии, самаритяне пришли в Палестину из Двуречья и приняли еврейскую "sv
                u8"религию и культуру. Сейчас существует не более 700 самаритян.</p>"sv },
    { "Syrc"sv, EcScriptType::CONSONANT, EcLangLife::ENDANGERED, EcWritingDir::RTL,
        u8"Сирийская"sv, u8"I в.",
        u8"сирийский <i>(исп. как литургический)</i>, новоарамейские, малаялам, согдийский"sv,
        u8"<p>Потомок арамейского алфавита, впоследствии развившийся в арабицу. Используется малыми семитскими народами.</p>"sv
            u8"<p>Тот вид письма, которым написанны раннехристианские рукописи, известен под названием эстрангело (греч. στρονγύλη\u00A0— круглое). "sv
                u8"Раскол сирийской церкви на несториан и яковитов привёл к разделению языка и письменности на две формы: "sv
                u8"восточносирийскую (несторианскую, халдейскую) и  западносирийскую (яковитскую, маронитскую). "sv
                u8"В них используются разные почерки и огласовки.</p>"sv },
    { "Thaa"sv, EcScriptType::ABUGIDA_COMBINING, EcLangLife::ALIVE, EcWritingDir::RTL,
        u8"Тана"sv, u8"XVIII век",
        u8"дивехи <i>(мальдивский язык)</i>"sv,
        u8"<p>Происхождение уникально: согласные произошли из арабских и индийских цифр, гласные\u00A0— из арабских огласовок. "sv
                u8"Со временем стали писать с изящным 45-градусным наклоном.</p>"sv
            u8"<p>В 1970-е годы тане грозило исчезновение: при президенте Ибрагиме Насире появилась телексная связь, и "sv
                u8"правительство перешло на латинскую транслитерацию. Новый президени Момун Абдул Гайюм восстановил тану, "sv
                u8"и обе письменности используются параллельно.</p>" },
    { "Zinh"sv, EcScriptType::NONE, EcLangLife::NOMATTER, EcWritingDir::NOMATTER,
        u8"Разные"sv, {}, {},
        u8"<p>Комбинирующая метка используется в нескольких разных письменностях.</p>" },
};



const uc::NumType uc::numTypeInfo[static_cast<int>(uc::EcNumType::NN)] {
    { ""sv },
    { u8"Цифра"sv },
    { u8"Особая цифра"sv },
    { u8"Число"sv },
};


const uc::BidiClass uc::bidiClassInfo[static_cast<int>(EcBidiClass::NN)] {
    { "DIR"sv, EcBidiStrength::EXPLICIT,    u8"Тэг направления"sv,
               u8"Форматирующие символы, корректирующие работу двунаправленного алгоритма Юникода"sv },
    { "AL"sv,  EcBidiStrength::STRONG,      u8"Арабская буква"sv,
               u8"Пишутся справа налево, переворачивают европейские цифры."sv },
    { "AN"sv,  EcBidiStrength::WEAK,        u8"Арабская цифра"sv,
               u8"Пишутся справа налево. Есть тонкие правила, связанные с уровнями вложенности." },

    { "B"sv,   EcBidiStrength::NEUTRAL,     u8"Разделитель абзацев"sv,
               u8"Каждый абзац обрабатывается двунаправленным алгоритмом по отдельности."sv },
    { "BN"sv,  EcBidiStrength::NEUTRAL,     u8"Неграфический"sv,
               u8"Управляющие, соединители и тэги, не имеющие своего написания." },
    { "CS"sv,  EcBidiStrength::WEAK,        u8"Цифровой разделитель"sv,
               u8"Оказавшись в арабском или европейском числе, эти символы становятся его частью." },

    { "EN"sv,  EcBidiStrength::WEAK,        u8"Европейская цифра"sv,
               u8"Европейские цифры в арабском тексте пишутся справа налево."sv },
    { "ES"sv,  EcBidiStrength::WEAK,        u8"Европейский ±"sv,
               u8"Символы + и − внутри европейского числа становятся его частью. Символ ± к этому классу не относится."sv },
    { "ET"sv,  EcBidiStrength::WEAK,        u8"Европейская единица измерения"sv,
               u8"Валютные и прочие символы, встречающиеся рядом с числами. Оказавшись рядом с европейским числом, они становятся частью числа."sv },

    { "L"sv,   EcBidiStrength::STRONG,      u8"Слева направо"sv,
               u8"Латиница, кириллица и прочие алфавиты с написанием слева направо."sv },
    { "NSM"sv, EcBidiStrength::WEAK,        u8"Непротяжённая метка"sv,
               u8"Непротяжённые и охватывающие комбинирующие метки. Направление написания\u00A0— как у основного символа."sv },
    { "ON"sv,  EcBidiStrength::NEUTRAL,     u8"Прочий нейтральный"sv,
               u8"Приспосабливается к направлению окружающего текста."sv },

    { "NM"sv,  EcBidiStrength::NEUTRAL,     u8"Нейтральный отзеркаливающийся"sv,
               u8"Скобки и похожие символы при написании справа налево отзеркаливаются."sv },
    { "R"sv,   EcBidiStrength::STRONG,      u8"Справа налево"sv,
               u8"Иврит, а также исторические письменности Ближнего Востока."sv },
    { "S"sv,   EcBidiStrength::NEUTRAL,     u8"Разделитель участков"sv,
               u8"Tab и некоторые другие символы. Посреди текстового абзаца встречаются редко, но обычно каждый участок обрабатывается двунаправленным алгоритмом по отдельности. Направление табуляции внутри абзаца постоянное."sv },
    { "WS"sv,  EcBidiStrength::NEUTRAL,     u8"Пробел"sv,
               u8"Нейтральны. Есть тонкие правила, касающиеся форматирования пробелами." },
    //{ u8"Ошибка"sv },  check for equal number
};


void uc::completeData()
{
    auto n = nCps();
    for (unsigned i = 0; i < n; ++i) {
        auto& cp = cpInfo[i];
        ++cp.bidiClass().nChars;
        ++cp.category().nChars;
        auto& script = cp.script();
        ++script.nChars;
        script.ecVersion = std::min(script.ecVersion, cp.ecVersion);
    }
}
