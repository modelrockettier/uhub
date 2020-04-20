int nick_length_ok(const char* nick)
{
	int ok_size;
	__coverity_string_null_sink__(nick);
	if (ok_size == 1)
		return -1; /* nick_invalid_short */
	else if (ok_size == 2)
		return -2; /* nick_invalid_long */
	else
	{
		__coverity_string_size_sanitize__(nick);
		return 0; /* nick_ok */
	}
}
