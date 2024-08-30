#include "Parser.hh"
#include "adt/logs.hh"

using namespace adt;

namespace json
{

static void expect(Parser*s, enum Token::TYPE t, adt::String svFile, int line);
static void next(Parser* s);
static void parseNode(Parser* s, Object* pNode);
static void parseIdent(Parser* s, TagVal* pTV);
static void parseNumber(Parser* s, TagVal* pTV);
static void parseObject(Parser* s, Object* pNode);
static void parseArray(Parser* s, Object* pNode); /* arrays are same as objects */
static void parseNull(Parser* s, TagVal* pTV);
static void parseBool(Parser* s, TagVal* pTV);

void
ParserLoad(Parser* s, adt::String path)
{
    s->sName = path;
    LexerLoadFile(&s->l, path);

    s->tCurr = LexerNext(&s->l);
    s->tNext = LexerNext(&s->l);

    if ((s->tCurr.type != Token::LBRACE) && (s->tCurr.type != Token::LBRACKET))
    {
        CERR("wrong first token\n");
        exit(2);
    }

    s->pHead = (Object*)adt::alloc(s->pAlloc, 1, sizeof(Object));
}

void
ParserParse(Parser* s)
{
    parseNode(s, s->pHead);
}

static void
expect(Parser* s, enum Token::TYPE t, adt::String svFile, int line)
{
    if (s->tCurr.type != t)
    {
        CERR("('%.*s', at %d): (%.*s): unexpected token: expected: '%c', got '%c'\n",
             svFile.size, svFile.pData, line, s->sName.size, s->sName.pData, char(t), char(s->tCurr.type));
        exit(2);
    }
}

static void
next(Parser* s)
{
    s->tCurr = s->tNext;
    s->tNext = LexerNext(&s->l);
}

static void
parseNode(Parser* s, Object* pNode)
{
    switch (s->tCurr.type)
    {
        default:
            next(s);
            break;

        case Token::IDENT:
            parseIdent(s, &pNode->tagVal);
            break;

        case Token::NUMBER:
            parseNumber(s, &pNode->tagVal);
            break;

        case Token::LBRACE:
            next(s); /* skip brace */
            parseObject(s, pNode);
            break;

        case Token::LBRACKET:
            next(s); /* skip bracket */
            parseArray(s, pNode);
            break;

        case Token::NULL_:
            parseNull(s, &pNode->tagVal);
            break;

        case Token::TRUE_:
        case Token::FALSE_:
            parseBool(s, &pNode->tagVal);
            break;
    }
}

static void
parseIdent(Parser* s, TagVal* pTV)
{
    *pTV = {.tag = TAG::STRING, .val {.sv = s->tCurr.sLiteral}};
    next(s);
}

static void
parseNumber(Parser* s, TagVal* pTV)
{
    bool bReal = adt::StringLastOf(s->tCurr.sLiteral, '.') != adt::NPOS;

    if (bReal)
        *pTV = {.tag = TAG::DOUBLE, .val = {.d = atof(s->tCurr.sLiteral.pData)}};
    else
        *pTV = TagVal{.tag = TAG::LONG, .val = {.l = atol(s->tCurr.sLiteral.pData)}};

    next(s);
}

static void
parseObject(Parser* s, Object* pNode)
{
    pNode->tagVal.tag = TAG::OBJECT;
    pNode->tagVal.val.o = adt::Array<Object> (s->pAlloc, 1);
    auto& aObjs = getObject(pNode);

    for (; s->tCurr.type != Token::RBRACE; next(s))
    {
        expect(s, Token::IDENT, __FILE__, __LINE__);
        Object ob {.svKey = s->tCurr.sLiteral, .tagVal = {}};
        adt::ArrayPush(&aObjs, ob);

        /* skip identifier and ':' */
        next(s);
        expect(s, Token::ASSIGN, __FILE__, __LINE__);
        next(s);

        parseNode(s, &adt::ArrayLast(&aObjs));

        if (s->tCurr.type != Token::COMMA)
        {
            next(s);
            break;
        }
    }

    if (aObjs.size == 0)
        next(s);
}

static void
parseArray(Parser* s, Object* pNode)
{
    pNode->tagVal.tag = TAG::ARRAY;
    pNode->tagVal.val.a = adt::Array<Object> (s->pAlloc, 1);
    auto& aTVs = getArray(pNode);

    /* collect each key/value pair inside array */
    for (; s->tCurr.type != Token::RBRACKET; next(s))
    {
        adt::ArrayPush(&aTVs, {});

        switch (s->tCurr.type)
        {
            default:
            case Token::IDENT:
                parseIdent(s, &adt::ArrayLast(&aTVs).tagVal);
                break;

            case Token::NULL_:
                parseNull(s, &adt::ArrayLast(&aTVs).tagVal);
                break;

            case Token::TRUE_:
            case Token::FALSE_:
                parseBool(s, &adt::ArrayLast(&aTVs).tagVal);
                break;

            case Token::NUMBER:
                parseNumber(s, &adt::ArrayLast(&aTVs).tagVal);
                break;

            case Token::LBRACE:
                next(s);
                parseObject(s, &adt::ArrayLast(&aTVs));
                break;
        }

        if (s->tCurr.type != Token::COMMA)
        {
            next(s);
            break;
        }
    }

    if (aTVs.size == 0)
        next(s);
}

static void
parseNull(Parser* s, TagVal* pTV)
{
    *pTV = {.tag = TAG::NULL_, .val = {nullptr}};
    next(s);
}

static void
parseBool(Parser* s, TagVal* pTV)
{
    bool b = s->tCurr.type == Token::TRUE_ ? true : false;
    *pTV = {.tag = TAG::BOOL, .val = {.b = b}};
    next(s);
}

void
ParserPrint(Parser* s)
{
    ParserPrintNode(s->pHead, "", 0);
    COUT("\n");
}

void
ParserPrintNode(Object* pNode, adt::String svEnd, int depth)
{
    adt::String key = pNode->svKey;

    switch (pNode->tagVal.tag)
    {
        default:
            break;

        case TAG::OBJECT:
            {
                auto& obj = getObject(pNode);
                adt::String q0, q1, objName0, objName1;

                if (key.size == 0)
                {
                    q0 = q1 = objName1 = objName0 = "";
                }
                else
                {
                    objName0 = key;
                    objName1 = ": ";
                    q1 = q0 = "\"";
                }

                COUT("%*s", depth, "");
                COUT("%.*s%.*s%.*s%.*s{\n", q0.size, q0.pData, objName0.size, objName0.pData, q1.size, q1.pData, objName1.size, objName1.pData);
                for (u32 i = 0; i < obj.size; i++)
                {
                    adt::String slE = (i == obj.size - 1) ? "\n" : ",\n";
                    ParserPrintNode(&obj[i], slE, depth + 2);
                }
                COUT("%*s", depth, "");
                COUT("}%.*s", svEnd.size, svEnd.pData);
            }
            break;

        case TAG::ARRAY:
            {
                auto& arr = getArray(pNode);
                adt::String q0, q1, arrName0, arrName1;

                if (key.size == 0)
                {
                    q0 =  q1 = arrName1 = arrName0 = "";
                }
                else
                {
                    arrName0 = key;
                    arrName1 = ": ";
                    q1 = q0 = "\"";
                }

                COUT("%*s", depth, "");

                if (arr.size == 0)
                {
                    COUT("%.*s%.*s%.*s%.*s[", q0.size, q0.pData, arrName0.size, arrName0.pData, q1.size, q1.pData, arrName1.size, arrName1.pData);
                    COUT("]%.*s", (int)svEnd.size, svEnd.pData);
                    break;
                }

                COUT("%.*s%.*s%.*s%.*s[\n", q0.size, q0.pData, arrName0.size, arrName0.pData, q1.size, q1.pData, arrName1.size, arrName1.pData);
                for (u32 i = 0; i < arr.size; i++)
                {
                    adt::String slE = (i == arr.size - 1) ? "\n" : ",\n";

                    switch (arr[i].tagVal.tag)
                    {
                        default:
                        case TAG::STRING:
                            {
                                adt::String sl = getString(&arr[i]);
                                COUT("%*s", depth + 2, "");
                                COUT("\"%.*s\"%.*s", sl.size, sl.pData, slE.size, slE.pData);
                            }
                            break;

                        case TAG::NULL_:
                                COUT("%*s", depth + 2, "");
                                COUT("%s%.*s", "null", slE.size, slE.pData);
                            break;

                        case TAG::LONG:
                            {
                                long num = getLong(&arr[i]);
                                COUT("%*s", depth + 2, "");
                                COUT("%ld%.*s", num, slE.size, slE.pData);
                            }
                            break;

                        case TAG::DOUBLE:
                            {
                                double dnum = getDouble(&arr[i]);
                                COUT("%*s", depth + 2, "");
                                COUT("%.17lf%.*s", dnum, slE.size, slE.pData);
                            }
                            break;

                        case TAG::BOOL:
                            {
                                bool b = getBool(&arr[i]);
                                COUT("%*s", depth + 2, "");
                                COUT("%s%.*s", b ? "true" : "false", slE.size, slE.pData);
                            }
                            break;

                        case TAG::OBJECT:
                                ParserPrintNode(&arr[i], slE, depth + 2);
                            break;
                    }
                }
                COUT("%*s", depth, "");
                COUT("]%.*s", (int)svEnd.size, svEnd.pData);
            }
            break;

        case TAG::DOUBLE:
            {
                /* TODO: add some sort formatting for floats */
                double f = getDouble(pNode);
                COUT("%*s", depth, "");
                COUT("\"%.*s\": %.17lf%.*s", key.size, key.pData, f, svEnd.size, svEnd.pData);
            }
            break;

        case TAG::LONG:
            {
                long i = getLong(pNode);
                COUT("%*s", depth, "");
                COUT("\"%.*s\": %ld%.*s", key.size, key.pData, i, svEnd.size, svEnd.pData);
            }
            break;

        case TAG::NULL_:
                COUT("%*s", depth, "");
                COUT("\"%.*s\": %s%.*s", key.size, key.pData, "null", svEnd.size, svEnd.pData);
            break;

        case TAG::STRING:
            {
                adt::String sl = getString(pNode);
                COUT("%*s", depth, "");
                COUT("\"%.*s\": \"%.*s\"%.*s", key.size, key.pData, sl.size, sl.pData, svEnd.size, svEnd.pData);
            }
            break;

        case TAG::BOOL:
            {
                bool b = getBool(pNode);
                COUT("%*s", depth, "");
                COUT("\"%.*s\": %s%.*s", key.size, key.pData, b ? "true" : "false", svEnd.size, svEnd.pData);
            }
            break;
    }
}

void
ParserTraverse(Parser*s, Object* pNode, bool (*pfn)(Object* p, void* args), void* args)
{
    if (pfn(pNode, args)) return;

    switch (pNode->tagVal.tag)
    {
        default: break;

        case TAG::ARRAY:
        case TAG::OBJECT:
            {
                auto& obj = getObject(pNode);

                for (u32 i = 0; i < obj.size; i++)
                    ParserTraverse(s, &obj[i], pfn, args);
            }
            break;
    }
}

} /* namespace json */
