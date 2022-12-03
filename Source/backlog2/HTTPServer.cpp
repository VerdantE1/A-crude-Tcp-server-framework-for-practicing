#include "HTTPServer.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <cstdarg>
#include <sys/uio.h>

const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the requested file.\n";
const char* doc_root = "/var/www/html";




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



bool HttpServer::HttpVersion_WriteFd(const TcpConnectionPtr& conn)
{
		Handler* handler = conn->GetHandler();
		int m_sockfd = handler->fd();

		int bytes_have_send = 0;
		int bytes_to_send = file_stat_.st_size;

		while (bytes_have_send < bytes_to_send)
		{
			int ret = 0;
			ret = writev(m_sockfd, m_iv, m_iv_count);
			if (ret <= -1)
			{
				if (errno == EAGAIN) //next loop write
				{
					handler->enableWriting();
					return true;
				}
				unmap(); //Error
				return false;
			}
			bytes_have_send += ret;

		}

		assert(bytes_have_send >= bytes_to_send);

		if (linger_)
		{
			init();
			//handler->enableReading();
			return true;
		}
		else
		{
			//handler->enableReading();
			return false;
		}

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
					
					return Do_Request(conn);
				}
				break;
			}
			case HttpServer::CHECK_STATE_CONTENT:
			{
				ret = Parse_Content(text,conn);
				if (ret == GET_REQUEST)
				{
					
					return Do_Request(conn);
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
	if (text[0] == '\0')
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
		text += strspn(text, " \t");
		if (strcasecmp(text, "keep-alive") == 0)
		{
			linger_ = true;
		}
	}

	/*Header:Content-Length*/
	else if (strncasecmp(text, "Content-Length:",15) == 0)
	{
		text += 15;
		text += strspn(text, " \t");
		content_length_ = atol(text);
	}


	/*Header:Host*/
	else if (strncasecmp(text, "Host:", 5) == 0)
	{
		text += 5;
		text += strspn(text, " \t");
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
static int cnt = 0;
HttpServer::HTTP_CODE HttpServer::Do_Request(const TcpConnectionPtr& conn)
{
	strcpy(real_file_, doc_root);

	int len = strlen(doc_root);
	cnt++;
	printf(
	strncpy(real_file_ + len, url_, FILENAME_LEN - static_cast<size_t>(len) - 1));

	if (stat(real_file_, &file_stat_) < 0)       //file not exist
	{
		return NO_RESOURCE;
	}

	if (!(file_stat_.st_mode & S_IROTH))
	{
		return FORBIDDEN_REQUET;
	}

	if (S_ISDIR(file_stat_.st_mode))
	{
		return BAD_REQUEST;
	}

	int fd = open(real_file_, O_RDONLY);

	m_file_address_ = (char*)mmap(0, file_stat_.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

	close(fd);
	return FILE_REQUEST;
}

void HttpServer::unmap()
{
	if (m_file_address_ != nullptr)
	{
		munmap(m_file_address_, file_stat_.st_size);
		m_file_address_ = 0;
	}
}

bool HttpServer::Process_Write(const TcpConnectionPtr& conn, HTTP_CODE ret)       //yeah,the begging of the dream hh 
{                         
	switch (ret)
	{
		case HttpServer::BAD_REQUEST:
		{
			Add_Status_Line(conn, 400, error_400_title);
			Add_Headers(conn, strlen(error_400_form));
			Add_Content(conn, error_400_form);
			break;
		}
		case HttpServer::NO_RESOURCE:
		{
			Add_Status_Line(conn, 404, error_404_title);
			Add_Headers(conn, strlen(error_404_form));
			Add_Content(conn, error_404_form);
			break;
		}
		case HttpServer::FORBIDDEN_REQUET:
		{
			Add_Status_Line(conn, 403, error_403_title);
			Add_Headers(conn, strlen(error_403_form));
			Add_Content(conn, error_403_form);
			break;
		}
		case HttpServer::FILE_REQUEST:
		{
			Add_Status_Line(conn, 200, ok_200_title);
			if (file_stat_.st_size != 0)
			{
				Add_Headers(conn, file_stat_.st_size);
				m_iv[0].iov_base = m_file_address_;
				m_iv[0].iov_len = file_stat_.st_size;
				m_iv_count = 1;
				return true;
			}
			else
			{
				const char* ok_string = "<html><body></body></html>";
				Add_Headers(conn,strlen(ok_string));
				Add_Content(conn, ok_string);

			}
		
		}
		case HttpServer::INTERNAL_ERROR:
		{
			Add_Status_Line(conn, 500, error_500_title);
			Add_Headers(conn, strlen(error_500_form));
			Add_Content(conn, error_500_form);
			break;
		}
		default:
		{
			return false;
		}


	}

	return true;
}

//bool HttpServer::Add_Response(const TcpConnectionPtr& conn,const char* format,...)
//{
//
//	return false;
//}

bool HttpServer::Add_Status_Line(const TcpConnectionPtr& conn, int status, const char* title)
{
	
	Buffer* buf = conn->GetoutputBuffer();
	char str[101]={
		'\0'
	};
	sprintf(str,"%s %d %s\r\n", "HTTP/1.1", status, title);
	buf->append(str, strlen(str));

	return true;
}

bool HttpServer::Add_Headers(const TcpConnectionPtr& conn, int ContentLen)
{
	
	Add_Length(conn,ContentLen);
	Add_Linger(conn);
	Add_Blank_Line(conn);
	return true;
}

bool HttpServer::Add_Length(const TcpConnectionPtr& conn, int ContentLen)
{
	Buffer* buf = conn->GetoutputBuffer();
	char str[101] = {
		'\0'
	};
	sprintf(str, ("Content-Length: %d\r\n"), ContentLen);
	buf->append(str, strlen(str));
	return true;
}

bool HttpServer::Add_Linger(const TcpConnectionPtr& conn)
{
	Buffer* buf = conn->GetoutputBuffer();
	char str[101] = {
		'\0'
	};
	sprintf(str,"Connection: %s\r\n",(linger_ == true)?"keep-alive":"close");
	buf->append(str, strlen(str));

	return true;
}

bool HttpServer::Add_Blank_Line(const TcpConnectionPtr& conn)
{
	Buffer* buf = conn->GetoutputBuffer();
	char str[101] = {
		'\0'
	};
	sprintf (str,"%s","\r\n");
	buf->append(str, strlen(str));
	return true;
}

bool HttpServer::Add_Content(const TcpConnectionPtr& conn, const char* Content)
{
	Buffer* buf = conn->GetoutputBuffer();
	char str[101] = {
		'\0'
	};
	sprintf(str,"%s",Content);
	buf->append(str, strlen(str));

	return true;
}

