int nick_length_ok(const char* nick)
{
	int len;
	if (len == 1)
		return -1; /* nick_invalid_short */
	else if (len == 2)
		return -2; /* nick_invalid_long */
	else
	{
		__coverity_string_size_sanitize__(nick);
		return 0; /* nick_ok */
	}
}
