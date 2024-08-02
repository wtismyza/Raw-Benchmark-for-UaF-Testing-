static int
pipe_open(const struct decompressor *dc, struct kbdfile *fp)
{
	char *pipe_cmd;

	pipe_cmd = malloc(strlen(dc->cmd) + strlen(fp->pathname) + 2);
	if (pipe_cmd == NULL)
		return -1;

	sprintf(pipe_cmd, "%s %s", dc->cmd, fp->pathname);

	fp->fd = popen(pipe_cmd, "r");
	fp->flags |= KBDFILE_PIPE;

	free(pipe_cmd);

	if (!(fp->fd)) {
		char buf[200];
		strerror_r(errno, buf, sizeof(buf));
		ERR(fp->ctx, "popen: %s: %s", pipe_cmd, buf);
		free(pipe_cmd);
		return -1;
	}

	return 0;
}
