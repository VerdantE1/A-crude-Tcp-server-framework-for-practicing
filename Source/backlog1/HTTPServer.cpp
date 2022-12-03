#include "HTTPServer.h"

void HttpServer::init()
{
	linger_ = false;
	url_ = 0;
	version_ = 0;
	method_ = GET;
	host_ = 0;
	content_length_ = 0;
	start_line_ = Buffer::kCheapPrepend;

}

HttpServer::LINE_STATUS HttpServer::Parse_Line(const TcpConnectionPtr& conn)
{
	char temp;
	Buffer* inputbuf = conn->GetinputBuffer();
	size_t& checked_idx = inputbuf->Getreadidx();
	size_t& read_idx = inputbuf->Getwriteidx(); 
	

	if (checked_idx == read_idx) return LINE_OPEN;          //No any char

	for (; checked_idx < read_idx; ++checked_idx)
	{
		temp = *inputbuf->peek();
		if (temp == '\r')
		{
			if ((checked_idx + 1) == read_idx)
			{
				return LINE_OPEN;
			}
			else if ((* inputbuf)[checked_idx + 1] == '\n')
			{
				(* inputbuf)[checked_idx++] = '\0';
				(* inputbuf)[checked_idx++] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
		else if (temp == '\n')
		{
			if ((*inputbuf)[checked_idx - 1] == '\r')
			{
				(*inputbuf)[checked_idx-1] = '\0';
				(*inputbuf)[checked_idx++] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
	}
	return LINE_OPEN;
	
}



HttpServer::HTTP_CODE HttpServer::Process_Read(const TcpConnectionPtr& conn)
{
	LINE_STATUS line_status = LINE_OK;
	HTTP_CODE ret = NO_REQUEST;
	
	char* text = 0;
	while (((check_state_ == CHECK_STATE_CONTENT) && (line_status == LINE_OK)) || ((line_status = Parse_Line(conn)) == LINE_OK))  
	{																															 
		text = get_line(conn);
		start_line_ = conn->GetinputBuffer()->Getreadidx();

		switch (check_state_)
		{
			case HttpServer::CHECK_STATE_REQUESTLINE:
			{
				ret = Parse_Request_Line(text,conn);
				if (ret == BAD_REQUEST)
				{
					return BAD_REQUEST;
				}
				break;
			}
			case HttpServer::CHECK_STATE_HEADER:
			{
				ret = Parse_Headers(text,conn);
				if (ret == BAD_REQUEST)
				{
					return BAD_REQUEST;
				}
				else if (ret == GET_REQUEST)
				{
					return GET_REQUEST;
					//return Do_Request();
				}
				break;
			}
			case HttpServer::CHECK_STATE_CONTENT:
			{
				ret = Parse_Content(text,conn);
				if (ret == GET_REQUEST)
				{
					return GET_REQUEST;
					//return Do_Request();
				}
				line_status = LINE_OPEN;
				break;
			}
			default:
			{
				return INTERNAL_ERROR;
			}
		}
	}
	return INTERNAL_ERROR;
	
}


HttpServer::HTTP_CODE HttpServer::Parse_Content(char* text, const TcpConnectionPtr& conn)           
{
	size_t m_read_idx = conn->Getinput_writeidx();
	size_t m_check_idx = conn->Getinput_readidx();
	if (m_read_idx >= (content_length_ + m_check_idx)) //Is Content all be read?
	{
		text[content_length_] = '\0'; 
		return GET_REQUEST;
	}
	return NO_REQUEST;
}



//Acquire 1:Method 2:URL 3:HTTPVersion
HttpServer::HTTP_CODE HttpServer::Parse_Request_Line(char* text, const TcpConnectionPtr& conn)
{
	url_ = strpbrk(text, " \t");
	if (!url_)
	{
		return BAD_REQUEST;
	}
	*url_++ = '\0';

	char* method = text;
	if (strcasecmp(method, "GET") == 0)
	{
		method_ = GET;
	}
	/**********
	else if(strcasecmp(method,"POST") == 0 )
	{

	}
	else if()
	{
	
	}	
	......                  More Method 
	******************************/
	else
	{
		return BAD_REQUEST;
	}


	url_ += strspn(url_, " \t");
	version_ = strpbrk(url_, " \t");
	if (!version_)
	{
		return BAD_REQUEST;
	}
	*version_++ = '\0';

	version_ += strspn(version_, " \t");
	if (strcasecmp(version_, "HTTP/1.1") != 0)
	{
		return BAD_REQUEST;
	}



	//parse url
	if (strcasecmp(url_, "http://") == 0)
	{
		url_ += 7;
		url_ = strchr(url_, '/');
	}
	if (!url_ || url_[0] != '/')
	{
		return BAD_REQUEST;
	}
	check_state_ = CHECK_STATE_HEADER;       //transfer mainstate machine's state

	return NO_REQUEST;
}

HttpServer::HTTP_CODE HttpServer::Parse_Headers(char* text, const TcpConnectionPtr& conn)
{
	if (text[0] == '\0 ')
	{
		//msg is read done,transfer MainStateMachine's State to CHECK_STATE_CONTENT
		if (content_length_ != 0)
		{
			check_state_ = CHECK_STATE_CONTENT;
			return NO_REQUEST;
		}
		return GET_REQUEST;
	}
	
	/*Header:Connection*/
	else if (strncasecmp(text, "Connection:", 11) == 0)
	{
		text += 11;
		text += strspn(text, "\t");
		if (strcasecmp(text, "keep-alive") == 0)
		{
			linger_ = true;
		}
	}

	/*Header:Content-Length*/
	else if (strncasecmp(text, "Content-Length:",15) == 0)
	{
		text += 15;
		text += strspn(text, "\t");
		content_length_ = atol(text);
	}


	/*Header:Host*/
	else if (strncasecmp(text, "Host:", 5) == 0)
	{
		text += 5;
		text += strspn(text, "\t");
		host_ = text;
	}

	else
	{
		printf("Sorry,Now I cant distinguish this headr text\n");
	}
	return NO_REQUEST;
}


char* HttpServer::get_line(const TcpConnectionPtr& conn)
{
	Buffer* inputbuf = conn->GetinputBuffer();
	size_t& checked_idx = inputbuf->Getreadidx();
	size_t& read_idx = inputbuf->Getwriteidx();
	
	return &(*inputbuf)[start_line_];
}