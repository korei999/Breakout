#include "Parser.hh"

#include "adt/logs.hh"

using namespace adt;

namespace json
{

#define OK_OR_RET(RES) if (RES == STATUS::FAIL) return STATUS::FAIL;

STATUS
Parser::parse(adt::String sJson)
{
    m_lex = Lexer(sJson);

    m_tCurr = m_lex.next();
    m_tNext = m_lex.next();

    if ((m_tCurr.eType != TOKEN_TYPE::L_BRACE) && (m_tCurr.eType != TOKEN_TYPE::L_BRACKET))
    {
        CERR("wrong first token\n");
        return STATUS::FAIL;
    }

    do
    {
        m_aObjects.push(m_pAlloc, {});
        if (parseNode(&m_aObjects.last()) == STATUS::FAIL)
        {
            LOG_WARN("parseNode() failed\n");
            return STATUS::FAIL;
        }
    }
    while (m_tCurr.eType == TOKEN_TYPE::L_BRACE); /* some json files have multiple root objects */

    return STATUS::OK;
}

STATUS
Parser::printNodeError()
{
    const auto& tok = m_tCurr;
    CERR("({}, {}): unexpected token: '{}'\n",
        tok.row, tok.column, m_tCurr.eType
    );
    return STATUS::FAIL;
}

void
Parser::next()
{
    m_tCurr = m_tNext;
    m_tNext = m_lex.next();
}

STATUS
Parser::expect(TOKEN_TYPE t)
{
    const auto& tok = m_tCurr;

    if (u32(tok.eType & t) > 0u)
    {
        return STATUS::OK;
    }
    else
    {
        CERR("({}, {}): unexpected token: expected: '{}', got '{}' ('{}')\n",
             tok.row, tok.column, t, m_tCurr.eType, m_tCurr.sLiteral
        );
        return STATUS::FAIL;
    }
}

STATUS
Parser::expectNot(TOKEN_TYPE t)
{
    const auto& tok = m_tCurr;

    if (u32(tok.eType & t) > 0u)
    {
        CERR("({}, {}): unexpected token: not expected: '{}', got '{}' ('{}')\n",
             tok.row, tok.column, t, m_tCurr.eType, m_tCurr.sLiteral
        );
        return STATUS::FAIL;
    }
    else return STATUS::OK;
}

STATUS
Parser::parseNode(Object* pNode)
{
    /*LOG_WARN("({}, {}): '{}': '{}'\n", m_tCurr.row, m_tCurr.column, m_tCurr.eType, m_tCurr.sLiteral);*/

    switch (m_tCurr.eType)
    {
        default:
            return printNodeError();
            break;

        case json::TOKEN_TYPE::QUOTED_STRING:
            parseIdent(&pNode->tagVal);
            break;

        case json::TOKEN_TYPE::STRING:
            parseString(&pNode->tagVal);
            break;

        case TOKEN_TYPE::NUMBER:
            parseNumber(&pNode->tagVal);
            break;

        case TOKEN_TYPE::FLOAT:
            parseFloat(&pNode->tagVal);
            break;

        case TOKEN_TYPE::L_BRACE:
            next(); /* skip brace */
            OK_OR_RET(parseObject(pNode));
            break;

        case TOKEN_TYPE::L_BRACKET:
            next(); /* skip bracket */
            OK_OR_RET(parseArray(pNode));
            break;
    }

    return STATUS::OK;
}

void
Parser::parseString(TagVal* pTV)
{
    const auto& sLit = m_tCurr.sLiteral;

    if (sLit == "null")
        *pTV = {.tag = TAG::NULL_, .val = {nullptr}};
    else if (sLit == "true")
        *pTV = {.tag = TAG::BOOL, .val = {.b = true}};
    else if (sLit == "false")
        *pTV = {.tag = TAG::BOOL, .val = {.b = false}};
    else
        *pTV = {.tag = TAG::STRING, .val {.s = m_tCurr.sLiteral}};

    next();
}

void
Parser::parseIdent(TagVal* pTV)
{
    *pTV = {.tag = TAG::STRING, .val {.s = m_tCurr.sLiteral}};
    next();
}

void
Parser::parseNumber(TagVal* pTV)
{
    *pTV = {.tag = TAG::LONG, .val = {.l = atoll(m_tCurr.sLiteral.m_pData)}};
    next();
}

void
Parser::parseFloat(TagVal* pTV)
{
    *pTV = {.tag = TAG::DOUBLE, .val = {.d = atof(m_tCurr.sLiteral.m_pData)}};
    next();
}

STATUS
Parser::parseObject(Object* pNode)
{
    pNode->tagVal.tag = TAG::OBJECT;
    pNode->tagVal.val.o = adt::VecBase<Object>(m_pAlloc, 2);
    auto& aObjs = getObject(pNode);

    while (m_tCurr.eType != TOKEN_TYPE::R_BRACE)
    {
        /* make sure key is quoted */
        OK_OR_RET(expect(TOKEN_TYPE::QUOTED_STRING));

        aObjs.push(m_pAlloc, {.sKey = m_tCurr.sLiteral, .tagVal = {}});

        /* skip identifier and ':' */
        next();
        OK_OR_RET(expect(TOKEN_TYPE::COLON));
        next();

        OK_OR_RET(parseNode(&aObjs.last()));

        if (m_tCurr.eType != TOKEN_TYPE::COMMA)
        {
            OK_OR_RET(expect(TOKEN_TYPE::R_BRACE));
            next();
            break;
        }
        else
        {
            next();
            OK_OR_RET(expectNot(TOKEN_TYPE::R_BRACE | TOKEN_TYPE::STRING));
        }
    }

    if (aObjs.getSize() == 0) next();

    return STATUS::OK;
}

STATUS
Parser::parseArray(Object* pNode)
{
    pNode->tagVal.tag = TAG::ARRAY;
    pNode->tagVal.val.a = adt::VecBase<Object>(m_pAlloc, 2);
    auto& aTVs = getArray(pNode);

    /* collect each key/value pair inside array */
    while (m_tCurr.eType != TOKEN_TYPE::R_BRACKET)
    {
        aTVs.push(m_pAlloc, {});

        switch (m_tCurr.eType)
        {
            default:
            case TOKEN_TYPE::QUOTED_STRING:
            case TOKEN_TYPE::STRING:
                parseString(&aTVs.last().tagVal);
                break;

            case TOKEN_TYPE::NUMBER:
                parseNumber(&aTVs.last().tagVal);
                break;

            case TOKEN_TYPE::FLOAT:
                parseFloat(&aTVs.last().tagVal);
                break;

            case TOKEN_TYPE::L_BRACE:
                next();
                OK_OR_RET(parseObject(&aTVs.last()));
                break;
        }

        if (m_tCurr.eType != TOKEN_TYPE::COMMA)
        {
            OK_OR_RET(expect(TOKEN_TYPE::R_BRACKET));
            next();
            break;
        }
        else
        {
            next();
            OK_OR_RET(expectNot(TOKEN_TYPE::L_BRACKET));
        }
    }

    if (aTVs.getSize() == 0) next();

    return STATUS::OK;
}

void
Parser::destroy()
{
    auto fn = +[](Object* p, void* a) -> bool {
        auto* pAlloc = (IAllocator*)a;

        if (p->tagVal.tag == TAG::ARRAY || p->tagVal.tag == TAG::OBJECT)
            pAlloc->free(p->tagVal.val.a.m_pData);

        return false;
    };

    traverseAll(fn, m_pAlloc);
    m_aObjects.destroy(m_pAlloc);
}

void
Parser::print(FILE* fp)
{
    for (auto& obj : m_aObjects)
    {
        printNode(fp, &obj, "", 0);
        fputc('\n', fp);
    }
}

void
printNode(FILE* fp, Object* pNode, adt::String svEnd, int depth)
{
    adt::String key = pNode->sKey;

    switch (pNode->tagVal.tag)
    {
        default: break;

        case TAG::OBJECT:
        {
            auto& obj = getObject(pNode);
            adt::String q0, q1, objName0, objName1;

            if (key.m_size == 0)
            {
                q0 = q1 = objName1 = objName0 = "";
            }
            else
            {
                objName0 = key;
                objName1 = ": ";
                q1 = q0 = "\"";
            }

            fprintf(fp, "%*s", depth, "");
            /*COUT("{: >{}}", "", depth);*/
            print::toFILE(fp, "{}{}{}{}{\n", q0, objName0, q1, objName1);
            for (u32 i = 0; i < obj.getSize(); i++)
            {
                adt::String slE = (i == obj.getSize() - 1) ? "\n" : ",\n";
                printNode(fp, &obj[i], slE, depth + 2);
            }
            fprintf(fp, "%*s", depth, "");
            /*COUT("{: >{}}", "", depth);*/
            print::toFILE(fp, "}{}", svEnd);
        }
        break;

        case TAG::ARRAY:
        {
            auto& arr = getArray(pNode);
            adt::String q0, q1, arrName0, arrName1;

            if (key.m_size == 0)
            {
                q0 =  q1 = arrName1 = arrName0 = "";
            }
            else
            {
                arrName0 = key;
                arrName1 = ": ";
                q1 = q0 = "\"";
            }

            fprintf(fp, "%*s", depth, "");
            /*COUT("{: >{}}", "", depth);*/

            if (arr.getSize() == 0)
            {
                print::toFILE(fp, "{}{}{}{}[", q0, arrName0, q1, arrName1);
                print::toFILE(fp, "]{}", svEnd);
                break;
            }

            print::toFILE(fp, "{}{}{}{}[\n", q0, arrName0, q1, arrName1);
            for (u32 i = 0; i < arr.getSize(); i++)
            {
                adt::String slE = (i == arr.getSize() - 1) ? "\n" : ",\n";

                switch (arr[i].tagVal.tag)
                {
                    default:
                    case TAG::STRING:
                    {
                        adt::String sl = getString(&arr[i]);
                        fprintf(fp, "%*s", depth + 2, "");
                        /*COUT("{: >{}}", "", depth + 2);*/
                        print::toFILE(fp, "\"{}\"{}", sl, slE);
                    }
                    break;

                    case TAG::NULL_:
                    fprintf(fp, "%*s", depth + 2, "");
                    /*COUT("{: >{}}", "", depth + 2);*/
                    print::toFILE(fp, "{}{}", "null", slE);
                    break;

                    case TAG::LONG:
                    {
                        long num = getLong(&arr[i]);
                        fprintf(fp, "%*s", depth + 2, "");
                        /*COUT("{: >{}}", "", depth + 2);*/
                        print::toFILE(fp, "{}{}", num, slE);
                    }
                    break;

                    case TAG::DOUBLE:
                    {
                        double dnum = getDouble(&arr[i]);
                        fprintf(fp, "%*s", depth + 2, "");
                        /*COUT("{: >{}}", "", depth + 2);*/
                        print::toFILE(fp, "{}{}", dnum, slE);
                    }
                    break;

                    case TAG::BOOL:
                    {
                        bool b = getBool(&arr[i]);
                        fprintf(fp, "%*s", depth + 2, "");
                        /*COUT("{: >{}}", "", depth + 2);*/
                        print::toFILE(fp, "{}{}", b ? "true" : "false", slE);
                    }
                    break;

                    case TAG::OBJECT:
                    printNode(fp, &arr[i], slE, depth + 2);
                    break;
                }
            }
            fprintf(fp, "%*s", depth, "");
            /*COUT("{: >{}}", "", depth);*/
            print::toFILE(fp, "]{}", svEnd);
        }
        break;

        case TAG::DOUBLE:
        {
            double f = getDouble(pNode);
            fprintf(fp, "%*s", depth, "");
            /*COUT("{: >{}}", "", depth);*/
            print::toFILE(fp, "\"{}\": {}{}", key, f, svEnd);
        }
        break;

        case TAG::LONG:
        {
            long i = getLong(pNode);
            fprintf(fp, "%*s", depth, "");
            /*COUT("{: >{}}", "", depth);*/
            print::toFILE(fp, "\"{}\": {}{}", key, i, svEnd);
        }
        break;

        case TAG::NULL_:
        fprintf(fp, "%*s", depth, "");
        /*COUT("{: >{}}", "", depth);*/
        print::toFILE(fp, "\"{}\": {}{}", key, "null", svEnd);
        break;

        case TAG::STRING:
        {
            adt::String sl = getString(pNode);
            fprintf(fp, "%*s", depth, "");
            /*COUT("{: >{}}", "", depth);*/
            print::toFILE(fp, "\"{}\": \"{}\"{}", key, sl, svEnd);
        }
        break;

        case TAG::BOOL:
        {
            bool b = getBool(pNode);
            fprintf(fp, "%*s", depth, "");
            /*COUT("{: >{}}", "", depth);*/
            print::toFILE(fp, "\"{}\": {}{}", key, b ? "true" : "false", svEnd);
        }
        break;
    }
}

void
Parser::traverse(Object* pNode, bool (*pfn)(Object* p, void* pFnArgs), void* pArgs)
{
    switch (pNode->tagVal.tag)
    {
        default: break;

        case TAG::ARRAY:
        {
            auto& obj = getArray(pNode);

            for (u32 i = 0; i < obj.getSize(); i++)
                traverse(&obj[i], pfn, pArgs);
        }
        break;

        case TAG::OBJECT:
        {
            auto& obj = getObject(pNode);

            for (u32 i = 0; i < obj.getSize(); i++)
                traverse(&obj[i], pfn, pArgs);
        }
        break;
    }

    if (pfn(pNode, pArgs)) return;
}

adt::VecBase<Object>&
Parser::getRoot()
{
    assert(m_aObjects.m_size > 0 && "[Parser]: this json is empty");

    if (m_aObjects.m_size == 1) return getObject(&m_aObjects.first());
    else return m_aObjects;
}

} /* namespace json */
